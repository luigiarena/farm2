/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: pool_manager.h
    Header per il pool manager. Descrive la struttura del pool con il suo mutex e la lista
    delle strutture che rappresentano i worker thread attivi. Espone le funzioni per 
    poter aggiungere nuovi worker o eliminarli conoscendone il pthread id
*/
#ifndef POOL_MANAGER_H
#define POOL_MANAGER_H

typedef struct worker {
    int id;                         // Id incrementale simbolico
    pthread_t tid;                  // Id del pthread
    struct worker *next;            // Puntatore al prossimo elemento
} worker_t;

typedef struct pool {
    pthread_mutex_t mtx;            // Mutex del pool
    int id_counter;                 // Contatore degli Id dei worker
    int counter;                    // Contatore dei Worker
    int nthread;                    // Numero iniziale di Worker
    worker_t *list;                 // Puntatore alla lista del pool
} pool_t;

int pool_manager(pool_t *pool);

pool_t *init_pool(int n);
void add_worker(pool_t *p);
int rem_worker(pool_t *p, pthread_t tid);
void free_pool(pool_t *p);

#endif