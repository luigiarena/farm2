/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: pool_manager.c
    Sorgente di pool manager. Si occupa della gestione del pool dei worker thread. Li
    crea, aspetta che terminino, liberando la memoria della propria struttura e della
    coda, che dovrebbe essere vuota - alla fine del suo ciclo -
*/
#define _POSIX_C_SOURCE 200809L

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
extern pool_t *pool;

/*
    Funzione che gestisce il pool chiamata da Masterworker
    Crea la lista dei worker thread attivi, ne aggiunge di nuovi se riceve il
    segnale usr1, attende la terminazione dei worker con lo svuotamento della
    coda, libera infine lo spazio allocato per coda e pool
    @return    active_workers  numero di worker attivi alla chiusura del pool    
*/
int pool_manager() {

    int active_workers = 0;

    //  Crea i worker thread iniziali
    for (int i = 0; i < pool->nthread; i++) {
        add_worker(pool);
        V_PRINT_ARG(MASTERWORKER, "avviato worker thread %d", pool->counter);
    }

    //  Pool manager va in loop aspettando le richieste di aggiunta worker e fine coda
    while(!no_more_files) {
        if (usr1_signal != 0) {
            add_worker(pool);
            usr1_signal--;
            V_PRINT_ARG(MASTERWORKER, "aggiunto nuovo worker thread %d", pool->counter);
        }
    }

    //  Pool manager cerca di fare join con i worker thread aperti e ne distrugge la lista
    //  (Non ho più bisogno di usare il mutex del pool qui)
    worker_t *temp = pool->list;
    while (temp != NULL) {
        if (pthread_join(temp->tid, NULL)) {
            fprintf(stderr, "MasterWorker error -> errore join worker: %d\n", temp->id);
            exit(EXIT_FAILURE);
        }
        temp = temp->next;
        active_workers++;
    }
    V_PRINT_MSG(MASTERWORKER, "pool manager ha effettuato la join con i worker rimasti attivi")

    //  Libera la memoria di pool e coda
    free_pool(pool);
    free_coda(coda);

    V_PRINT_MSG(MASTERWORKER, "pool manager ha liberato la memoria e termina")

    //  Ritorna il numero di worker attivi alla fine del pool
    return active_workers;
}

/*
    Inizializza il pool con il suo mutex
    @param    n  numero iniziale di worker thread
    @return   p  puntatore al pool creato
*/
pool_t *init_pool(int n) {

    pool_t *p = malloc(sizeof(pool_t));
    ec_val(coda, NULL, "errore allocazione pool");
    pthread_mutex_init(&p->mtx, NULL);
    p->id_counter = 0;
    p->counter = 0;
    p->nthread = n;
    p->list = NULL;

    return p;
}

/*
    Aggiunge un nuovo worker alla lista del pool, avviandone il thread associato
    @param    p  puntatore al pool
*/
void add_worker(pool_t *p) {

    worker_t *w = malloc(sizeof(worker_t));

    pthread_mutex_lock(&p->mtx);
    w->next = p->list;
    w->id = (p->id_counter)+1;

    if (pthread_create(&w->tid, NULL, &worker_thread, &w->id) != 0) {
        fprintf(stderr, "errore pthread_create worker: %d\n", w->id);
        pthread_mutex_unlock(&p->mtx);
        exit(EXIT_FAILURE);
    }
    
    p->list = w;
    p->id_counter++;
    p->counter++;
    pthread_mutex_unlock(&p->mtx);

    return;
}

/*
    Rimuove un worker dalla lista del pool conoscendone il suo tid
    @param    p    puntatore al pool
              tid  pthread id del worker
    @return   0    se la rimozione ha avuto successo
              -1   altrimenti
*/
int rem_worker(pool_t *p, pthread_t tid) {

    worker_t *w;
    worker_t *prev;

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

/*
    Libera la memoria della lista dei worker
    @param    w  puntatore alla lista dei worker
*/
void free_worker_list(worker_t *w) {

    if (w == NULL) return;
    free_worker_list(w->next);
    free(w);

    return;
}

/*
    Libera la memoria del pool
    @param    p  puntatore al pool
*/
void free_pool(pool_t *p) {

    free_worker_list(p->list);
    pthread_mutex_destroy(&p->mtx);
    free(p);
    
    return;
}
