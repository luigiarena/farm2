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
#include "pool_manager.h"
#include "utility.h"

extern volatile sig_atomic_t stop_signal;
extern volatile sig_atomic_t usr1_signal;
extern volatile sig_atomic_t usr2_signal;

extern int verbose;

extern coda_t *coda;

//extern int no_more_files;
int trova_id(pool_t *p, pthread_t tid);

// Funzione eseguita da ogni worker thread
void* worker_thread(void* arg) {
    mask_signals_worker();

    pthread_t tid = pthread_self();
    //int id = 0;
    //int id = trova_id(p, tid);
    //ec_val(id, 0, "Errore recupero id worker");

    V_PRINT_ARG(WORKER, "(%ld) avviato", tid);
      //sleepTime(500);
    char *path = malloc(PATH_MAX_LEN);
    while (!stop_signal) {
        //V_PRINT_ARG(WORKER, "(%d) sta eseguendo...", id);
        //leggi_coda(p->coda);
        // AGGIUNGI CONTROLLO PER USR2
        //sleep(1);
        printf("Worker %ld cerca di leggere coda\n", tid);
        //leggi_coda(coda);
        //if (coda->counter != 0) path = leggi_coda(coda);
        path = leggi_coda(coda);
        if (strcmp(path, "") == 0) {
            //printf("PATH NULLOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n");
            scrivi_coda(coda, "");
            break;
        }
        /*
        if (strcmp(path, "FINE")) {
            printf("Worker %ld HA PRESO LA FINE!\n", tid);
            scrivi_coda(coda, "FINE");
            break;
        }
        */
        printf("Letto------------------------->: %s\n", path);
        //sleepTime(500);
    }

    V_PRINT_ARG(WORKER, "(%ld) terminato", tid);
    
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

int trova_id(pool_t *p, pthread_t tid) {
    worker_t *w = malloc(sizeof(worker_t));
    int id = 0;
    printf("TROVA_ID cerca LOCK\n");
    pthread_mutex_lock(&p->mtx);
    printf("TROVA_ID prende LOCK\n");
/*    w = p->list;
    while (w != NULL && w->tid != tid) w = w->next;
*/    if (w != NULL) id = w->id;
    pthread_mutex_unlock(&p->mtx);
    printf("TROVA_ID rilascia LOCK\n");
    return id;
}