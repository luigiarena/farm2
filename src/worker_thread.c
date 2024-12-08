#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "worker_thread.h"
#include "pool_list.h"

extern volatile sig_atomic_t stop_signal;
extern volatile sig_atomic_t usr1_signal;
extern volatile sig_atomic_t usr2_signal;

extern int coda_vuota;
extern int no_more_files;

extern Worker_list lista;
extern Coda coda_concorrente;

// Funzione eseguita da ogni worker thread
void* worker_thread(void* arg) {
    mask_signals_worker();

    Coda *cc = (Coda *) arg;

    int id = pthread_self();

    while (!stop_signal && !no_more_files) {
        printf("Worker %d: Eseguendo...\n", id);

        pthread_mutex_lock(&cc->lock);
        if (&cc->not_empty && no_more_files) {
            pthread_mutex_unlock(&cc->lock);
            break;
        }
        while (cc->size == 0) {
            pthread_cond_wait();
        }

        char *path_file = pop_file(cc);

        pthread_cond_signal(&cc->not_full);
        pthread_mutex_unlock(&cc->lock);

        long result = calcola_res(path_file);
        if (result == -1) return NULL;
    }
    
    return NULL;
}

void mask_signals_worker() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGPIPE);
    sigaddset(&mask, SIGUSR1);

    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
        perror("Thread Worker error -> maschera segnali");
    }
}

long calcola_res (char *path_file){
    FILE *fp;
    // apertura il file in modalità "rb"
    fp = fopen(path_file, "rb");
    if (fp == NULL) {
        printf("Errore nell'apertura del file %s\n", path_file);
        //correggere il return
        return -1;
    }
    // posizionamento alla fine del file
    fseek(fp, 0L, SEEK_END);
    // ottenere posizione corrente (che è la dimensione del file)
    long size = ftell(fp);
    size = size / sizeof(long);
    long vals[size];

    fseek(fp, 0L, SEEK_SET);
    // lettura dei valori dal file e inserimento nell'array
    int count = fread(vals, sizeof(long), size, fp);
    fclose(fp);
    if(count != size){
        //non ho letto il numero corretto di elementi del file
        printf("Errore lettura long\n");
        exit(1);
    }
    //se la lettura è andata a buon fine eseguo il calcolo di result
    long result = 0, i;
    for(i=0;i<count;i++){
        result+=(i*vals[i]);
    }
    return result;
}