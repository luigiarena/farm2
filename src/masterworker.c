/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: masterworker.c
    Descrizione: 
*/

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
#include "coda.h"
#include "utility.h"

//#define SOCKET_PATH			"./farm2.sck"
//#define BUF_MAX_SIZE                  255
#define MAX_NCONN                      10

extern int verbose;

volatile sig_atomic_t stop_signal = 0;
volatile sig_atomic_t usr_counter = 0;
pthread_mutex_t usr_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

//Worker_list *lista_w;
Coda *coda_concorrente;
int no_more_files = 0;

pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pool_cond = PTHREAD_COND_INITIALIZER;

static void *handler_signals(void *arg);
void riempi_coda(Coda *q, char *file_list[], int file_num, long tdelay, char *dname);
void esplora_dir(Coda *q, long tdelay, char *dname);

void masterWorker_main(char *file_list[], int file_num, int nthread, int qlen, long tdelay, char *dname) {
    V_PRINT_ARG(MASTERWORKER, "PID: %d", getpid());

    //pthread_t worker_pool[nthread];

/*
    // Gestore segnali per il MasterWorker
    signal(SIGHUP, handler_signals);
    //signal(SIGINT, handler_signals);
    signal(SIGQUIT, handler_signals);
    signal(SIGTERM, handler_signals);
    signal(SIGUSR1, handler_signals);
    signal(SIGUSR2, handler_signals);
*/
	// Gestisco segnali per MasterWorker
	sigset_t mask;

    ec_val(sigemptyset(&mask), -1, "Masterworker sigemptyset mask");

    ec_val(sigaddset(&mask, SIGHUP), -1, "Masterworker sigaddset sighup");
    ec_val(sigaddset(&mask, SIGINT), -1, "Masterworker sigaddset sigint");
    ec_val(sigaddset(&mask, SIGQUIT), -1, "Masterworker sigaddset sigquit");
    ec_val(sigaddset(&mask, SIGTERM), -1, "Masterworker sigaddset sigterm");
    ec_val(sigaddset(&mask, SIGUSR1), -1, "Masterworker sigaddset sigusr1");
    ec_val(sigaddset(&mask, SIGUSR2), -1, "Masterworker sigaddset sigusr2");

    // Ignoro SIGPIPE
    struct sigaction saction;
    memset(&saction, 0, sizeof(saction));
    saction.sa_handler = SIG_IGN;
    ec_val(sigaction(SIGPIPE, &saction, NULL), -1, "Masterworker sigaction ignore");

    // Applico sigmask
    ec_not(pthread_sigmask(SIG_BLOCK, &mask, NULL), 0, "Masterworker set sigmask");

    // Creo un thread detached che gestisce i segnali
    pthread_t handlerThread;
    ec_not(pthread_create(&handlerThread, NULL, &handler_signals, &mask), 0, "Masterworker pthread_create");
    ec_not(pthread_detach(handlerThread), 0, "Masterworker pthread_detach");

    // Creazione socket
    int server_socket;
    //int n_workers = nthread;

    struct sockaddr_un sa;
    char buffer[BUF_MAX_SIZE];

    server_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("MasterWorker error -> creazione socket fallita\n");
        exit(EXIT_FAILURE);
    }

        // Configurazione socket
        sa.sun_family = AF_UNIX;
        strcpy(sa.sun_path, SOCKET_PATH);

        int tentativi=0;
        V_PRINT_MSG(MASTERWORKER, "tentativo di connessione");
        // Connessione al server (collector)
        while (tentativi<MAX_NCONN && (connect(server_socket, (struct sockaddr *)&sa, sizeof(sa)) == -1)) {
            perror("MasterWorker error -> connessione fallita");
            close(server_socket);
            exit(EXIT_FAILURE);
            tentativi++;
            sleep(1);
        }

        V_PRINT_MSG(MASTERWORKER, "connessione con Collector stabilita!")

        // Attesa messaggio di conferma dal collector
        //read(server_socket, buffer, BUF_MAX_SIZE);
        //printf("Master ha ricevuto: %s\n", buffer);

        // Creo la coda concorrente
        coda_concorrente = create_coda(qlen);
        /*
        int test_push = push_coda(coda_concorrente, "Prova prova prova");
        printf("Test push: %d\n", test_push);
        printf("Test pop: %s\n", pop_coda(coda_concorrente));
        */

        Worker_list *lista_w;
        lista_w = malloc(sizeof(Worker_list));
        lista_w->count_w = 0;
        lista_w->head = NULL;

        // Creo i worker thread
        for (int i = 0; i < nthread; i++) {
            add_worker(lista_w);
        }

        riempi_coda(coda_concorrente, file_list, file_num, tdelay, dname);

        stampa_coda(coda_concorrente);

/*
        // FACCIO COSE
        //int index = 0;
        //int max_ciclo = 30;
        int ind_ciclo = 0;
        //while(!stop_signal && ind_ciclo<=max_ciclo)
        while(!stop_signal)
        {
            ind_ciclo++;
            //printf("Sono dentro il ciclo di masterworker\n");
            // RIVEDERE
            
            if (usr_counter > 0) {
                add_worker(lista_w);
                usr_counter--;
            }
            if (usr_counter < 0) {
                // DA FINIRE
                //del_worker();
                usr_counter++;
            }

            if (!no_more_files && coda_concorrente.size < qlen) {
                // Aggiungo prima i file inseriti come argomenti e
                // poi quelli contenuti nella directory indicata
            printf("Sono dentro il ciclo di aggiunta file in cc\n");
            printf("Lunghezza coda: %d\n", coda_concorrente.size);
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
*/
        // Uso optind per gestire tutti gli argomenti che non sono stati riconosciuti come parametri
        // Qui riempio la coda concorrente, se il file è regolare lo inserisco altrimenti lo ignoro
        /*
        for (; optind < argc; optind++) {      
            printf("extra arguments: %s\n", argv[optind]);
            push_file(&coda_concorrente, argv[optind]);  
        }
        */
/*
        int len = coda_concorrente.size;
        printf("Coda size: %d\n", coda_concorrente.size);
        Nodo *test = coda_concorrente.head;
        while (test != NULL) {
            printf("Iter file: %s\n", test->file_path);
            test = test->next;
        }
*/
        /*
        for (int i=0; i<len; i++) {
            printf("Verifica file: %s\n", pop_file(&coda_concorrente));
        }
        */

        V_PRINT_MSG(MASTERWORKER, "prima della join");

        // Aspetto la fine della coda concorrente

        Worker_node *iterator = NULL;
        // Attendo la terminazione dei worker
        iterator = lista_w->head;
        for (int i = 0; (i < lista_w->count_w) && (iterator!=NULL); i++) {
            if (pthread_join(iterator->tid, NULL)) {
                printf("Worker %ld chiuso\n", (lista_w->head+i)->tid);
                fprintf(stderr, "MasterWorker error -> errore join worker: %d\n", i);
                exit(EXIT_FAILURE);
            }
            iterator = iterator->next;
        }
        free(iterator);

        V_PRINT_MSG(MASTERWORKER, "dopo della join");

        // Invio messaggio "STOP"
        send(server_socket, "STOP", strlen("STOP"), 0);
        memset(buffer, 0, BUF_MAX_SIZE);
        read(server_socket, buffer, BUF_MAX_SIZE);
        V_PRINT_ARG(MASTERWORKER, "ha ricevuto %s", buffer);

        // Chiusura connessione e cancellazione del socket
        close(server_socket);
        unlink(SOCKET_PATH);

        // Salvo su file il numero di thread worker attivi
        save_nworkers(lista_w, "nworkeratexit.txt");

        free_coda(coda_concorrente);
        free_list(lista_w);
        V_PRINT_MSG(MASTERWORKER, "chiusura");

    return;
}

// Funzione per la gestione dei segnali in MasterWorker
static void *handler_signals(void *arg) {

    int sig;
    while(!stop_signal) {
        if (sigwait((sigset_t *)arg, &sig) != 0) {
            perror("fatal error: sigwait");
            return NULL;
        }
        switch(sig) {
            case SIGHUP:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGHUP\n", 34);
                stop_signal = 1;
                break;
            case SIGINT:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGINT\n", 34);
                stop_signal = 1;
                break;
            case SIGQUIT:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGQUIT\n", 35);
                //stop_signal = 1;
                pop_coda(coda_concorrente);
                printf("usr_counter prima: %d\n", usr_counter);
                pthread_mutex_lock(&usr_counter_mutex);
                usr_counter++;
                pthread_mutex_unlock(&usr_counter_mutex);
                printf("usr_counter dopo: %d\n", usr_counter);
                break;
            case SIGTERM:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGTERM\n", 35);
                stop_signal = 1;
                break;
            case SIGUSR1:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGUSR1\n", 35);
                pthread_mutex_lock(&usr_counter_mutex);
                usr_counter++;
                pthread_mutex_unlock(&usr_counter_mutex);
                break;
            case SIGUSR2:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGUSR2\n", 35);
                pthread_mutex_lock(&usr_counter_mutex);
                usr_counter++;
                pthread_mutex_unlock(&usr_counter_mutex);
                break;
            default:
                break;
        }
    }
    return NULL;
}

// Funzione che esplora la directory, saltando file ., .. e nascosti
void riempi_coda(Coda *q, char *file_list[], int file_num, long tdelay, char *dname) {
    int index = 0;
    FILE *new_file;
    // Inserisco prima la lista dei file passati come argomenti
    while (index < file_num && !stop_signal) {
        if (q->len == q->max) 
        {
            //printf("salto il ciclo di inserimento\n");
            continue;
        }
        else {
            // Devo fare controlli, devo bloccare la coda
            new_file = fopen(file_list[index], "r");
            ec_val(new_file, NULL, "Errore apertura file");
            fclose(new_file);
            // Attendo il ritardo tdelay
            sleepTime(tdelay);
            push_coda(q, file_list[index]);
            // TEST STAMPA CALCOLO
            printf("Test calcolo %s: %ld\n", file_list[index], calcola_res(file_list[index]));
            index++;
        }
    }
    // Poi esploro la directory se è stata passata
    if (dname != NULL) esplora_dir(q, tdelay, dname);

    no_more_files = 1;
    return;
}
void esplora_dir(Coda *q, long tdelay, char *dname) {
    struct dirent *entry;
    struct stat file_stat;

    DIR *dir = opendir(dname);
    if (!dir) {
        perror("Errore nell'aprire la directory dei file di input");
        return;
    }

    while (!stop_signal && (entry = readdir(dir)) != NULL) {
        while (!stop_signal && q->len == q->max) continue;

        char full_path[PATH_MAX_LEN];

        // Salta "." e ".." e i file nascosti
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Crea il path completo
        int path_len = strlen(dname)+strlen(entry->d_name) + 2;
        snprintf(full_path, path_len, "%s/%s", dname, entry->d_name);

        // Ottieni informazioni sul file
        if (stat(full_path, &file_stat) == -1) {
            perror("Errore nell'ottenere informazioni sul file");
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            // Se è una directory la esplora ricorsivamente
            //--printf("Directory: %s\n", full_path);
            esplora_dir(q, tdelay, full_path);
        } else if (S_ISREG(file_stat.st_mode)) {
            // Se è un file regolare lo aggiungo alla coda concorrente
            //printf("File regolare: %s\n", full_path);
            // Attendo il ritardo tdelay
            sleepTime(tdelay);
            push_coda(q, full_path);
            // TEST STAMPA CALCOLO
            printf("Test stampa full_path: %s\n", full_path);
            printf("Test calcolo %s: %ld\n", full_path, calcola_res(full_path));
        }
    }

    closedir(dir);
}
/*
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

char *pop_file(Coda *c) {
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
*/
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
    // w->tid = i;

    // Creo il nuovo worker thread
    if (pthread_create(&w->tid, NULL, worker_thread, coda_concorrente)) {
        fprintf(stderr, "MasterWorker error -> errore creazione worker %d\n", i);
        exit(EXIT_FAILURE);
    }

}

void rem_worker(Worker_list *l) {

}

void free_list(Worker_list *l) {
    
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
