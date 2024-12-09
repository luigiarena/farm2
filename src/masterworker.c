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
#include "pool_manager.h"

#define SOCKET_PATH			"./farm2.sck"
#define BUF_MAX_SIZE                  255
#define MAX_NCONN                      10

volatile sig_atomic_t stop_signal = 0;
volatile sig_atomic_t usr1_signal = 0;
volatile sig_atomic_t usr2_signal = 0;

Worker_list *lista_w;
Coda coda_concorrente;
int coda_piena = 0;
int coda_vuota = 0;
int no_more_files = 0;

pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pool_cond = PTHREAD_COND_INITIALIZER;

void MasterWorker(char *file_list[], int list_index, int nthread, int qlen, char *dname) {
    printf("Sono MasterWorker (PID: %d)\n", getpid());

    int server_socket;
    //int n_workers = nthread;

    struct sockaddr_un sa;
    char buffer[BUF_MAX_SIZE];

    //pthread_t worker_pool[nthread];

    // Gestore segnali per il MasterWorker
    signal(SIGHUP, handler_signals);
    //signal(SIGINT, handler_signals);
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
        for (int i = 0; i < nthread; i++) {
            add_worker(lista_w);
        }

        // FACCIO COSE
        int index = 0;
        while(!stop_signal)
        {
            if (usr1_signal != 0) {
                add_worker(lista_w);
                usr1_signal = 0;
            }
            if (usr2_signal != 0) {
                // DA FINIRE
                //del_worker();
                usr2_signal = 0;
            }

            if (!no_more_files && coda_concorrente.size < qlen) {
                // Aggiungo prima i file inseriti come argomenti e
                // poi quelli contenuti nella directory indicata
                if (index < list_index) {
                    push_file(&coda_concorrente, file_list[index]);
                    index++;
                } else {
                    no_more_files = 1;
                }                
            }

            //printf("MasterWorker -> ciclo\n");
            //sleep(1);
        }

        // Uso optind per gestire tutti gli argomenti che non sono stati riconosciuti come parametri
        // Qui riempio la coda concorrente, se il file è regolare lo inserisco altrimenti lo ignoro
        /*
        for (; optind < argc; optind++) {      
            printf("extra arguments: %s\n", argv[optind]);
            push_file(&coda_concorrente, argv[optind]);  
        }
        */

        int len = coda_concorrente.size;
        printf("Coda size: %d\n", coda_concorrente.size);
        Nodo *test = coda_concorrente.head;
        while (test != NULL) {
            printf("Iter file: %s\n", test->file_path);
            test = test->next;
        }
        /*
        for (int i=0; i<len; i++) {
            printf("Verifica file: %s\n", pop_file(&coda_concorrente));
        }
        */

        printf("MasterWorker -> prima join\n");

        // Aspetto la fine della coda concorrente

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

        print_nworkers(lista_w);
        free_lista(lista_w);
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
            stop_signal = 1;
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
            // Se è una directory la esplora ricorsivamente
            //--printf("Directory: %s\n", full_path);
            naviga_dir(full_path);
        } else if (S_ISREG(file_stat.st_mode)) {
            // Se è un file regolare lo aggiungo alla coda concorrente
            printf("File regolare: %s\n", full_path);
        }
    }

    closedir(dir);
}

int push_file(Coda *c, char *path) {
    Nodo *n = malloc(sizeof(Nodo));

    n->file_path = malloc(BUF_MAX_SIZE);
    strncpy(n->file_path, path, strlen(path));
    n->file_path[strlen(path)+1] = '\0';
    n->next = NULL;

    if (c->head == NULL) 
        c->head = n;
    else if (c->tail == NULL) 
        c->head->next = c->tail = n;
    else { 
        c->tail->next = n;
        c->tail = c->tail->next;
    }

    return c->size++;
}

char* pop_file(Coda *c) {
    char* path = malloc(BUF_MAX_SIZE);

    if (c->head == NULL) return NULL;
    else {
        strncpy(path, c->head->file_path, strlen(c->head->file_path));
        path[strlen(c->head->file_path)+1] = '\0';

        c->head = c->head->next;
        if (c->head == NULL) {
            c->tail = NULL;
            //c->not_empty = NULL; //test
        }   

        c->size--;
    }

    return path;
}

void add_worker(Worker_list *l) {
    Worker_node *w;
    Worker_node *temp;
    int i = l->count_w + 1;

    if ( (w = malloc(sizeof(Worker_node))) == NULL ) {
        fprintf(stderr, "MasterWorker error -> errore allocazione worker %d\n", i);
        exit(EXIT_FAILURE);
    } else {
        w->next = NULL;
        temp = l->head;
        if (temp == NULL) {
            temp = w;
        } else {
            while (temp->next != NULL) temp = temp->next;
            temp->next = w;
        }
    }
    l->count_w = i;

    // Creo il nuovo worker thread
    if (pthread_create(&w->tid, NULL, worker_thread, &coda_concorrente)) {
        fprintf(stderr, "MasterWorker error -> errore creazione worker %d\n", i);
        exit(EXIT_FAILURE);
    }

}

void rem_worker(Worker_list *l) {

}

void print_nworkers(Worker_list *l) {
    printf("Numero di Thread Worker alla chiusura: %d\n", l->count_w);
    return;
}

void free_lista(Worker_list *l) {
    
    if (l == NULL) return;
    else {
        free_nodo(l->head);
        free(l);
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