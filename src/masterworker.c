/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: masterworker.c
    Sorgente del file masterworker, descrive la sua funzione, quella che gestisce gran parte
    del programma. Contiene anche le funzione che masterworker usa nel suo ciclo vitale
*/
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "masterworker.h"
#include "explorer.h"
#include "coda.h"
#include "pool_manager.h"
#include "utility.h"

#define MAX_NCONN                      10   // Numero massimo di tentativi di connessione

extern int verbose;

// Variabili atomiche per la gestione dei segnali ricevuti
volatile sig_atomic_t stop_signal = 0;
volatile sig_atomic_t usr1_signal = 0;
volatile sig_atomic_t usr2_signal = 0;

// Variabile che indica quando sono finiti i file da aggiungere alla coda
volatile sig_atomic_t no_more_files = 0;

// Mutex per la gestione condivisa della socket tra i thread
int server_socket;
pthread_mutex_t socket_mtx;

// Coda Concorrente e Pool globali, sono condivisi tra tutti i thread generati da Masterthread
coda_t *coda;
pool_t *pool;

static void *handler_signals(void *arg);
void save_nworkers(int n, char *file);

// Funzione main di Masterworker
void masterWorker_main(master_data_t *data) {
    V_PRINT_ARG(MASTERWORKER, "PID: %d", getpid());

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
    pthread_t handler_tid;
    if (pthread_create(&handler_tid, NULL, handler_signals, &mask) != 0) {
        perror("Masterworker -> errore durante la creazione di handler thread");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_detach(handler_tid) != 0) {
        perror("Masterworker -> errore detached su handler thread");
        exit(EXIT_FAILURE);
    }
    
    // Creazione socket
    //int server_socket;
    struct sockaddr_un server_addr;
    //char buffer[BUF_MAX_SIZE];

    server_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("MasterWorker error -> creazione socket fallita\n");
        exit(EXIT_FAILURE);
    }

    // Configurazione del socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    V_PRINT_MSG(MASTERWORKER, "tentativo di stabilire una connessione");
    // Connessione al server (collector)
    int tentativi=0;
    while (tentativi<MAX_NCONN && (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)) {
        perror("MasterWorker error -> connessione fallita");
        close(server_socket);
        exit(EXIT_FAILURE);
        tentativi++;
        sleepTime(1000);
    }

    // Inizializzazione del mutex della socket
    pthread_mutex_init(&socket_mtx, NULL);

    V_PRINT_MSG(MASTERWORKER, "fine configurazione connessione")

    // Crea la coda concorrente
    coda = init_coda(data->qlen);

    // Crea il pool dei Worker
    pool = init_pool(data->nthread);

    // Avvia il thread Explorer che si occupererà di riempire la coda
    V_PRINT_MSG(MASTERWORKER, "avviato explorer per il riempimento della coda");

    pthread_t explorer_tid;
    if (pthread_create(&explorer_tid, NULL, explorer, data) != 0) {
        perror("Masterworker -> errore durante la creazione di explorer");
        exit(EXIT_FAILURE);
    }
    
    V_PRINT_ARG(MASTERWORKER, "thread explorer avviato: %ld\n", explorer_tid);

    // Avvia funzione che gestisce il pool e ritorna il numero di worker attivi
    int active_workers = pool_manager();

    V_PRINT_MSG(MASTERWORKER, "attende il join con explorer");

    // Attende la chiusura di Explorer
    if (pthread_join(explorer_tid, NULL)) {
        fprintf(stderr, "MasterWorker -> errore join explorer\n");
        exit(EXIT_FAILURE);
    }

    V_PRINT_MSG(MASTERWORKER, "explorer è stato terminato");
    
    //printf("Provo segnale stop: %d\n", stop_signal);
    if (!stop_signal) {
        pthread_kill(handler_tid, SIGTERM);
        //printf("segnale lanciato\n");
    }

    V_PRINT_MSG(MASTERWORKER, "handler è stato terminato");

    // Invia messaggio di terminazione a Collector
/*
    send(server_socket, "STOP", strlen("STOP"), 0);
    memset(buffer, 0, BUF_MAX_SIZE);
    read(server_socket, buffer, BUF_MAX_SIZE);
    V_PRINT_ARG(MASTERWORKER, "ha ricevuto %s", buffer);
*/
    pthread_mutex_lock(&socket_mtx);
    if (send(server_socket, "", 0, 0) == -1) {
        perror("invio chiusura connessione");
        pthread_mutex_unlock(&socket_mtx);
    }
    pthread_mutex_unlock(&socket_mtx);

    // Chiusura connessione e cancellazione del socket
    close(server_socket);
    pthread_mutex_destroy(&socket_mtx);

    // Salvo su file il numero di thread worker attivi
    save_nworkers(active_workers, "nworkeratexit.txt");

    V_PRINT_MSG(MASTERWORKER, "chiusura");

    // Libera la memoria della struttura dei dati di Masterworker
    //free_data(data);

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
                stop_signal = 1;
                break;
            case SIGTERM:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGTERM\n", 35);
                stop_signal = 1;
                break;
            case SIGUSR1:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGUSR1\n", 35);
                usr1_signal++;
                break;
            case SIGUSR2:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGUSR2\n", 35);
                usr2_signal++;
                break;
            default:
                break;
        }
    }
    //printf("Handler esce\n");
    pthread_exit(NULL);
}

// Salva il numero n dentro file, creandolo se non esiste e sovrascrivendolo nel caso
void save_nworkers(int n, char *file) {

    FILE *fp = fopen(file, "w");
    ec_val(fp, NULL, "Errore apertura file nworker");
    fprintf(fp, "%d\n", n);
    fclose(fp);

    return;
}

// Libera lo spazio dedicato alla struttura Data
void free_data(master_data_t *data) {
    if (data == NULL) return;
    //printf("free di data null\n");
    for (int i=0; i<data->num_file; i++) {
        free(data->file_list[i]);
    }
    free(data->file_list);
    free(data->dname);
    free(data);
    return;
}