/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: worker_thread.c
    Descrizione: 
*/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "worker_thread.h"
#include "coda.h"
#include "utility.h"

extern volatile sig_atomic_t stop_signal;
extern volatile sig_atomic_t usr1_signal;
extern volatile sig_atomic_t usr2_signal;

extern int verbose;

extern int coda_vuota;
extern int no_more_files;

//Worker_list *lista_w;
extern Coda coda_concorrente;

// Funzione eseguita da ogni worker thread
void* worker_thread(void* arg) {
    mask_signals_worker();

    //Coda *cc = (Coda *) arg;

    int id = pthread_self();
    //int id = lista_w->count_w;

    while (!stop_signal && !no_more_files) {
        V_PRINT_ARG(WORKER, "(%d) sta eseguendo...", id);
/*
        pthread_mutex_lock(&cc->lock);
        if (&cc->not_empty && no_more_files) {
            pthread_mutex_unlock(&cc->lock);
            break;
        }
*/
        // AGGIUNGI CONTROLLO PER USR2

        /*
        while (cc->size == 0) {
            //pthread_cond_wait();
        }
        */

        /*
        char *path_file = pop_file(cc);

        pthread_cond_signal(&cc->not_full);
        pthread_mutex_unlock(&cc->lock);

        long result = calcola_res(path_file);
        if (result == -1) return NULL;
        */
    }

    V_PRINT_ARG(WORKER, "(%d) termina", id);
    
    pthread_exit(NULL);
}

void mask_signals_worker() {
    sigset_t set;
    ec_val(sigemptyset(&set), -1, "Worker sigemptyset mask");

    ec_val(sigaddset(&set, SIGHUP), -1, "Worker sigaddset sighup");
    ec_val(sigaddset(&set, SIGINT), -1, "Worker sigaddset sigint");
    ec_val(sigaddset(&set, SIGQUIT), -1, "Worker sigaddset sigquit");
    ec_val(sigaddset(&set, SIGTERM), -1, "Worker sigaddset sigterm");
    ec_val(sigaddset(&set, SIGUSR1), -1, "Worker sigaddset sigurs1");
    ec_val(sigaddset(&set, SIGUSR2), -1, "Worker sigaddset sigusr2");
    
    ec_not(pthread_sigmask(SIG_BLOCK, &set, NULL), 0, "Worker set sigmask");

    // Ignoro SIGPIPE
    struct sigaction saction;
    memset(&saction, 0, sizeof(saction));
    saction.sa_handler = SIG_IGN;
    ec_val(sigaction(SIGPIPE, &saction, NULL), -1, "Worker sigaction ignore");
}

long calcola_res (char *path_file){
    FILE *fp;
    // Apre il file in modalità "rb"
    fp = fopen(path_file, "rb");
    if (fp == NULL) {
        printf("Errore nell'apertura del file %s\n", path_file);
        return -1;
    }
    // Posizionamento alla fine del file
    fseek(fp, 0L, SEEK_END);
    // Ottiene posizione corrente (che è la dimensione del file)
    long size = ftell(fp);
    size = size / sizeof(long);
    long vals[size];

    fseek(fp, 0L, SEEK_SET);
    // Lettura dei valori dal file e inserimento nell'array
    int count = fread(vals, sizeof(long), size, fp);
    fclose(fp);
    if(count != size){
        // Non ha letto il numero corretto di elementi del file
        printf("Errore lettura long\n");
        exit(1);
    }
    // Se la lettura è andata a buon fine esegue il calcolo di result
    long result = 0, i;
    for(i=0;i<count;i++){
        result+=(i*vals[i]);
    }
    return result;
}

// Salva il numero di worker su file
void save_nworkers(Worker_list *l, char *name_file) {
    printf("Stampa su file numero di Thread Worker alla chiusura: %d\n", l->count_w);
    FILE *fp = fopen(name_file, "w");
    ec_val(fp, NULL, "Errore apertura file nworker");
    fprintf(fp, "%d\n", l->count_w);
    fclose(fp);

    return;
}