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

#define MAX_NCONN                      10  //  Numero massimo di tentativi di connessione

extern int verbose;

//  Variabili atomiche per la gestione dei segnali ricevuti
volatile sig_atomic_t stop_signal = 0;
volatile sig_atomic_t usr1_signal = 0;
volatile sig_atomic_t usr2_signal = 0;

//  Variabile che indica quando sono finiti i file da aggiungere alla coda
volatile sig_atomic_t no_more_files = 0;

//  Socket condivisa e mutex per la sua gestione tra i worker thread
int server_socket;
pthread_mutex_t socket_mtx;

//  Coda Concorrente e Pool globali, sono condivisi tra tutti i thread generati da Masterthread
coda_t *coda;
pool_t *pool;

//  Firma delle funzioni utilizzate internamente da masterworker
static void *handler_signals(void *arg);
void save_nworkers(int n, char *file);

/*
    Funzione main di Masterworker
    @param    data  puntatore alla struttura data passata da farm
*/
void masterWorker_main(master_data_t *data) {

    //  Masterworker stampa il proprio PID per segnalare il suo avvio corretto
    V_PRINT_ARG(MASTERWORKER, "si avvia - PID: %d", getpid());

	//  Gestisce segnali per MasterWorker
	sigset_t mask;

    ec_val(sigemptyset(&mask), -1, "Masterworker sigemptyset mask");

    ec_val(sigaddset(&mask, SIGHUP), -1, "Masterworker sigaddset sighup");
    ec_val(sigaddset(&mask, SIGINT), -1, "Masterworker sigaddset sigint");
    ec_val(sigaddset(&mask, SIGQUIT), -1, "Masterworker sigaddset sigquit");
    ec_val(sigaddset(&mask, SIGTERM), -1, "Masterworker sigaddset sigterm");
    ec_val(sigaddset(&mask, SIGUSR1), -1, "Masterworker sigaddset sigusr1");
    ec_val(sigaddset(&mask, SIGUSR2), -1, "Masterworker sigaddset sigusr2");

    //  Ignora SIGPIPE
    struct sigaction saction;
    memset(&saction, 0, sizeof(saction));
    saction.sa_handler = SIG_IGN;
    ec_val(sigaction(SIGPIPE, &saction, NULL), -1, "Masterworker sigaction ignore");

    //  Applica sigmask
    ec_not(pthread_sigmask(SIG_BLOCK, &mask, NULL), 0, "Masterworker set sigmask");

    //  Crea un thread detached che gestisce i segnali
    pthread_t handler_tid;
    ec_not(pthread_create(&handler_tid, NULL, handler_signals, &mask), 0,
        "Masterworker -> errore durante la creazione di handler thread");

    ec_not(pthread_detach(handler_tid), 0, 
        "Masterworker -> errore detached su handler thread");
    V_PRINT_MSG(MASTERWORKER, "avviato thread handler signals");

    //  Creazione socket
    struct sockaddr_un server_addr;

    server_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    ec_val(server_socket, -1, "MasterWorker error -> creazione socket fallita\n");

    //  Configurazione del socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    V_PRINT_MSG(MASTERWORKER, "tentativo di stabilire una connessione");
    //  Connessione al server (collector)
    int tentativi=0;
    while (tentativi<MAX_NCONN && (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)) {
        perror("MasterWorker error -> connessione fallita");
        close(server_socket);
        exit(EXIT_FAILURE);
        tentativi++;
        sleepTime(1000);
    }

    //  Inizializzazione del mutex della socket
    pthread_mutex_init(&socket_mtx, NULL);
    V_PRINT_MSG(MASTERWORKER, "configurata connessione")

    //  Crea la coda concorrente
    coda = init_coda(data->qlen);
    V_PRINT_MSG(MASTERWORKER, "inizializzata coda concorrente");

    //  Crea il pool dei Worker
    pool = init_pool(data->nthread);
    V_PRINT_MSG(MASTERWORKER, "inizializzato pool dei worker");

    //  Avvia il thread Explorer che si occupererà di riempire la coda
    pthread_t explorer_tid;
    ec_not(pthread_create(&explorer_tid, NULL, explorer, data), 0,
        "Masterworker -> errore durante la creazione di explorer");
    V_PRINT_MSG(MASTERWORKER, "avviato thread explorer");

    //  Avvia funzione che gestisce il pool e ritorna il numero di worker attivi
    V_PRINT_MSG(MASTERWORKER, "passa il flusso di lavoro a pool manager");
    int active_workers = pool_manager();

    //  Attende la chiusura di Explorer
    ec_not(pthread_join(explorer_tid, NULL), 0, "MasterWorker -> errore join explorer");
    V_PRINT_MSG(MASTERWORKER, "effettuata join con explorer");
    
    //  Se il programma finisce senza nessun segnale ne lancio uno per
    //  chiudere correttamente l'handler in attesa
    if (!stop_signal) pthread_kill(handler_tid, SIGTERM);
    V_PRINT_MSG(MASTERWORKER, "handler è stato terminato");

    //  Invia messaggio di terminazione a Collector
    V_PRINT_MSG(MASTERWORKER, "invio messaggio di chiusura al server");
    pthread_mutex_lock(&socket_mtx);
    if (send(server_socket, "", 0, 0) == -1) {
        perror("invio chiusura connessione");
        pthread_mutex_unlock(&socket_mtx);
    }
    pthread_mutex_unlock(&socket_mtx);

    //  Chiusura connessione e cancellazione del socket
    close(server_socket);
    pthread_mutex_destroy(&socket_mtx);

    //  Salvo su file il numero di thread worker attivi
    save_nworkers(active_workers, "nworkeratexit.txt");
    V_PRINT_MSG(MASTERWORKER, "numero dei worker attivi salvato su file");

    V_PRINT_MSG(MASTERWORKER, "chiusura");

    return;
}

/*
    Funzione per la gestione dei segnali in MasterWorker
    Si interrompe solo quando riceve un segnale adeguato
    @param    arg  puntatore a void, castato in un puntatore a sigset_t
*/
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
    pthread_exit(NULL);
}

/*
    Salva il numero n dentro un file, sovrascrivendolo o creandolo se non esiste
    @param    n     intero da salvare
              file  puntatore al file
*/void save_nworkers(int n, char *file) {

    FILE *fp = fopen(file, "w");
    ec_val(fp, NULL, "Errore apertura file nworkeratexit");
    fprintf(fp, "%d\n", n);
    fclose(fp);

    return;
}

/*
    Libera lo spazio dedicato alla struttura Data
    @param    data  puntatore alla struttura data
*/
void free_data(master_data_t *data) {

    if (data == NULL) return;
    for (int i=0; i<data->num_file; i++) {
        free(data->file_list[i]);
    }
    free(data->file_list);
    free(data->dname);
    free(data);

    return;
}