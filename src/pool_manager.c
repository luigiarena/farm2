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
extern volatile sig_atomic_t usr2_signal;
extern volatile sig_atomic_t no_more_files;

extern coda_t *coda;

int pool_manager(pool_t *pool) {
    V_PRINT_MSG(MASTERWORKER, "pool manager partito\n");

    // Crea i worker thread iniziali
    for (int i = 0; i < pool->nthread; i++) {
        add_worker(pool);
        //printf("Numero di worker attivi: %d\n", pool->counter);
    }

    // Pool manager va in loop aspettando le richieste di aggiuta worker e la terminazione
    while(!stop_signal) {
        //sleepTime(500);
        //if (coda->counter != 0) printf("Letto: %s\n", leggi_coda(coda));
        //printf("Pool Manager aspetta fine\n");
        if (no_more_files) {
            printf("------------------------------------------------Ok sono dentro\n");
            stop_signal = 1;
            //scrivi_coda(coda, "-1");
        }
        if (usr1_signal != 0) {
            add_worker(pool);
            usr1_signal--;
        }
    }

    // Pool manager cerca di fare join con i worker thread aperti e ne distrugge la lista
    worker_t *temp = pool->list;

    int active_workers = 0;
    printf("Tentativo di join da parte di pool_manager con i worker\n");
    while (pool->list != NULL) {
        //printf("Entro nel ciclo di join di pool\n");
        //pthread_mutex_lock(&pool->mtx);
        //printf("Cerco di joinare il worker: %ld\n", pool->list->tid);
        if (pthread_join(pool->list->tid, NULL)) {
            fprintf(stderr, "MasterWorker error -> errore join worker: %d\n", pool->list->id);
            exit(EXIT_FAILURE);
        }
        printf("Worker %d - %ld chiuso\n", pool->list->id, pool->list->tid);
        temp = pool->list;
        pool->list = pool->list->next;
        free(temp);
        //pthread_mutex_unlock(&pool->mtx);
        active_workers++;
    }

    printf("POOL MANAGER STA PER TERMINARE\n");

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

    printf("ADD_WORKER cerca LOCK\n");
    pthread_mutex_lock(&p->mtx);
    printf("ADD_WORKER prende LOCK\n");

    w->next = p->list;
    w->id = (p->id_counter)+1;

    if (pthread_create(&w->tid, NULL, &worker_thread, p) != 0) {
        fprintf(stderr, "errore pthread_create worker: %d\n", w->id);
        pthread_mutex_unlock(&p->mtx);
        exit(EXIT_FAILURE);
    }

    printf("AGGIUNGENDO WORKER: %d - %ld\n", w->id, w->tid);
    
    p->list = w;
    p->id_counter++;
    p->counter++;
    pthread_mutex_unlock(&p->mtx);
    printf("ADD_WORKER rilascia LOCK\n");
}

int rem_worker(pool_t *p, pthread_t tid) {
    worker_t *w = malloc(sizeof(worker_t));
    worker_t *prev = malloc(sizeof(worker_t));
    printf("REM_WORKER cerca LOCK\n");
    pthread_mutex_trylock(&p->mtx);
    if (p->counter == 1) {
        pthread_mutex_unlock(&p->mtx);
        printf("REM_WORKER rilascia LOCK\n");
        return -1;
    }
    printf("REM_WORKER prende LOCK\n");
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
    printf("REM_WORKER rilascia LOCK\n");
    return 0;
}