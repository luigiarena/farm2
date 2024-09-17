#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include <dirent.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
//#include <sys/types.h>
//#include <sys/wait.h>

#include "masterworker.h"
#include "worker_thread.h"
#include "pool_list.h"

#define SOCKET_PATH			"./farm2.sck"
#define BUF_MAX_SIZE                  255
#define MAX_NCONN                      10

volatile sig_atomic_t stop_signal = 0;
volatile sig_atomic_t usr1_signal = 0;
volatile sig_atomic_t usr2_signal = 0;

Worker_list *lista_w;
Coda coda_concorrente;

pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pool_cond = PTHREAD_COND_INITIALIZER;

void MasterWorker(char *argv[], int argc, int optind, int nthread, int qlen, char *dname) {
    printf("Sono MasterWorker (PID: %d)\n", getpid());

    int control_master = 1;
    int server_socket;
    //int n_workers = nthread;

    struct sockaddr_un sa;
    char buffer[BUF_MAX_SIZE];

    //pthread_t worker_pool[nthread];

    // Gestore segnali per il MasterWorker
    signal(SIGHUP, handler_signals);
    signal(SIGINT, handler_signals);
    signal(SIGQUIT, handler_signals);
    signal(SIGTERM, handler_signals);
    signal(SIGUSR1, handler_signals);
    signal(SIGUSR2, handler_signals);

    // Creazione socket
    server_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("MasterWorker error -> creazione socket fallita\n");
        exit(EXIT_FAILURE);
    }

        // Configurazione socket
        sa.sun_family = AF_UNIX;
        strcpy(sa.sun_path, SOCKET_PATH);
/*
        // Connessione al Collector
        if (connect(server_socket, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
            perror("MasterWorker error -> connessione fallita");
            close(server_socket);
            exit(EXIT_FAILURE);
        } else printf("Tutto OK!\n");
*/
        int tentativi=0;
        printf("MasterWorker -> tento la connessione\n");
        // Connessione al server (collector)
        while (tentativi<MAX_NCONN && (connect(server_socket, (struct sockaddr *)&sa, sizeof(sa)) == -1)) {
            perror("MasterWorker error -> connessione fallita");
            close(server_socket);
            exit(EXIT_FAILURE);
            tentativi++;
            sleep(1);
        }

        printf("MasterWorker -> connessione con Collector stabilita!\n");

        // Attesa messaggio di conferma dal collector
        //read(server_socket, buffer, BUF_MAX_SIZE);
        //printf("Master ha ricevuto: %s\n", buffer);

        // Creo la coda concorrente
        coda_concorrente = crea_coda();

        lista_w = malloc(sizeof(Worker_list));
        lista_w->count_w = 0;
        lista_w->head = NULL;

        // Creo i worker thread
        /*
        for (int i = 0; i < nthread; i++) {
            if (pthread_create(&worker_pool[i], NULL, worker_thread, &coda_concorrente)) {
                fprintf(stderr, "MasterWorker error -> errore creazione worker %d\n", i);
                exit(EXIT_FAILURE);
            }
        }
        */
        for (int i = 0; i < nthread; i++) {
            add_worker();
        }

        // FACCIO COSE
        while(!stop_signal)
        {
            if (usr1_signal != 0) {
                add_worker();
                usr1_signal = 0;
            }
            if (usr2_signal != 0) {

                usr2_signal = 0;
            }
            printf("MasterWorker -> cicl\n");
            sleep(1);
        }

        // Uso optind per gestire tutti gli argomenti che non sono stati riconosciuti come parametri
        // Qui riempio la coda concorrente, se il file è regolare lo inserisco altrimenti lo ignoro
        for (; optind < argc; optind++) {      
            printf("extra arguments: %s\n", argv[optind]);  
        } 

        printf("MasterWorker -> prima join\n");



        Worker_node *iterator = NULL;
        // Attendo la terminazione dei worker
        iterator = lista_w->head;
        for (int i = 0; (i < lista_w->count_w) && (iterator!=NULL); i++) {
            if (pthread_join(iterator->tid, NULL)) {
                printf("Workerd %ld chiuso\n", (lista_w->head+i)->tid);
                fprintf(stderr, "MasterWorker error -> errore join worker: %d\n", i);
                exit(EXIT_FAILURE);
            }
            iterator = iterator->next;
        }
        free(iterator);

        printf("MasterWorker -> dopo join\n");

        // Invio messaggio "STOP"
        send(server_socket, "STOP", strlen("STOP"), 0);
        memset(buffer, 0, BUF_MAX_SIZE);
        read(server_socket, buffer, BUF_MAX_SIZE);
        printf("Master ha ricevuto: %s\n", buffer);

        // Chiusura connessione
        close(server_socket);

        print_nworkers();
        free_lista();
        printf("Chiusura di Masterworker\n");

    return;
}

// Funzione per la gestione dei segnali in MasterWorker
void handler_signals(int sig_rec) {
    switch(sig_rec) {
        case SIGHUP:
            write(1, "MasterWorker: ricevuto SIGHUP\n", 31);
            stop_signal = 1;
            break;
        case SIGINT:
            write(1, "MasterWorker: ricevuto SIGINT\n", 31);
            stop_signal = 1;
            break;
        case SIGQUIT:
            write(1, "MasterWorker: ricevuto SIGQUIT\n", 32);
            usr1_signal = 1;
            break;
        case SIGTERM:
            write(1, "MasterWorker: ricevuto SIGTERM\n", 32);
            stop_signal = 1;
            break;
        case SIGUSR1:
            write(1, "MasterWorker: ricevuto SIGUSR1\n", 32);
            usr1_signal = 1;
            break;
        case SIGUSR2:
            write(1, "MasterWorker: ricevuto SIGUSR2\n", 32);
            usr2_signal = 1;
            break;
        default:
            break;
    }
}

// Funzione che esplora la directory, saltando file ., .. e nascosti
void naviga_dir(const char *dname) {
    struct dirent *entry;
    struct stat file_stat;

    DIR *dir = opendir(dname);
    if (!dir) {
        perror("Errore nell'aprire la directory dei file di input");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[1024];

        // Salta "." e ".." e i file nascosti
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Crea il path completo
        snprintf(full_path, sizeof(full_path), "%s/%s", dname, entry->d_name);

        // Ottieni informazioni sul file
        if (stat(full_path, &file_stat) == -1) {
            perror("Errore nell'ottenere informazioni sul file");
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            // Se è una directory la esplorala ricorsivamente
            //--printf("Directory: %s\n", full_path);
            naviga_dir(full_path);
        } else if (S_ISREG(file_stat.st_mode)) {
            // Se è un file regolare lo aggiungo alla coda concorrente
            printf("File regolare: %s\n", full_path);
        }
    }

    closedir(dir);
}

void add_worker() {
    Worker_node *w;
    Worker_node *temp;
    int i = lista_w->count_w + 1;

    if ( (w = malloc(sizeof(Worker_node))) == NULL ) {
        fprintf(stderr, "MasterWorker error -> errore allocazione worker %d\n", i);
        exit(EXIT_FAILURE);
    } else {
        w->next = NULL;
        temp = lista_w->head;
        if (temp == NULL) {
            temp = w;
        } else {
            while (temp->next != NULL) temp = temp->next;
            temp->next = w;
        }
    }
    lista_w->count_w = i;

    // Creo il nuovo worker thread
    if (pthread_create(&w->tid, NULL, worker_thread, &coda_concorrente)) {
        fprintf(stderr, "MasterWorker error -> errore creazione worker %d\n", i);
        exit(EXIT_FAILURE);
    }

}

void print_nworkers() {
    printf("Numero di Thread Worker alla chiusura: %d\n", lista_w->count_w);
    return;
}

void free_lista() {
    
    if (lista_w == NULL) return;
    else {
        free_nodo(lista_w->head);
        free(lista_w);
        return;
    }
}

void free_nodo(Worker_node *w) {
    if (w == NULL) return;
    else {
        free_nodo(w->next);
        free(w);
        return;
    }

}
/*
void free_lista() {
    Worker_node *w = lista_w->head;
    Worker_node *temp = lista_w->head;
    //int n = lista_w->count_w;

    while (w != NULL) {
        temp = w;
        w = w->next;
        free(&temp);
    }
    free(lista_w);
    return;
}
*/