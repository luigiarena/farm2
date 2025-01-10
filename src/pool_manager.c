/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: pool_manager.c
    Descrizione: 
*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>

#include "utility.h"
#include "pool_manager.h"
#include "worker_thread.h"
#include "coda.h"

extern int verbose;

extern volatile sig_atomic_t stop_signal;
extern volatile sig_atomic_t usr1_signal;

extern volatile sig_atomic_t no_more_files;

extern coda_t *coda;

volatile sig_atomic_t worker_control = 0;

int pool_manager(pool_t *pool) {
    V_PRINT_MSG(MASTERWORKER, "avvio di pool manager\n");

    // Crea i worker thread iniziali
    for (int i = 0; i < pool->nthread; i++) {
        add_worker(pool);
        V_PRINT_ARG(MASTERWORKER, "avviato worker thread %d", pool->counter);
    }

    // Pool manager va in loop aspettando le richieste di aggiuta worker e la fine dei file in input
    while(!stop_signal) {
        if (no_more_files) stop_signal = 1;

        if (usr1_signal != 0) {
            add_worker(pool);
            usr1_signal--;
            V_PRINT_ARG(MASTERWORKER, "aggiunto worker thread %d", pool->counter);
        }
    }

    // Pool manager cerca di fare join con i worker thread aperti e ne distrugge la lista
    worker_t *temp = pool->list;

    int active_workers = 0;
    V_PRINT_MSG(MASTERWORKER, "pool manager attende chiusura dei worker thread")
    while (pool->list != NULL) {
        if (pthread_join(pool->list->tid, NULL)) {
            fprintf(stderr, "MasterWorker error -> errore join worker: %d\n", pool->list->id);
            exit(EXIT_FAILURE);
        }
        temp = pool->list;
        pool->list = pool->list->next;
        free(temp);

        active_workers++;
    }

    //printf("Active_worker: %d - coda->counter: %d\n", active_workers, coda->counter);
    //if (active_workers != coda->counter) fprintf(stderr, "Problema nel conteggio dei worker all'uscita\n");

    free_coda(coda);

    V_PRINT_MSG(MASTERWORKER, "terminazione di pool manager")

    return active_workers;
}

pool_t *init_pool(int n) {
    pool_t *p = malloc(sizeof(pool_t));
    pthread_mutex_init(&p->mtx, NULL);
    p->id_counter = 0;
    p->counter = 0;
    p->nthread = n;
    p->list = NULL;
    return p;
}

void add_worker(pool_t *p) {
    worker_t *w = malloc(sizeof(worker_t));

    pthread_mutex_lock(&p->mtx);
    w->next = p->list;
    w->id = (p->id_counter)+1;

    if (pthread_create(&w->tid, NULL, &worker_thread, p) != 0) {
        fprintf(stderr, "errore pthread_create worker: %d\n", w->id);
        pthread_mutex_unlock(&p->mtx);
        exit(EXIT_FAILURE);
    }
    
    p->list = w;
    p->id_counter++;
    p->counter++;
    pthread_mutex_unlock(&p->mtx);
}

int rem_worker(pool_t *p, pthread_t tid) {
    worker_t *w = malloc(sizeof(worker_t));
    worker_t *prev = malloc(sizeof(worker_t));

    pthread_mutex_trylock(&p->mtx);
    if (p->counter == 1) {
        pthread_mutex_unlock(&p->mtx);
        return -1;
    }

    prev = p->list;
    w = p->list;
    if (w != NULL && w->tid == tid) p->list = p->list->next;
    else {
        w = w->next;
        while (w != NULL && w->tid != tid) {
            prev = w;
            w = w->next;
        }
        prev->next = w->next;
    }
    p->counter--;
    pthread_mutex_unlock(&p->mtx);
    return 0;
}