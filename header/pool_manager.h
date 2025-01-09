/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: pool_manager.h
    Descrizione: 
*/
#ifndef POOL_MANAGER_H
#define POOL_MANAGER_H

typedef struct worker {
    int id;
    pthread_t tid;
    struct worker *next;
} worker_t;

typedef struct pool {
    pthread_mutex_t mtx;
    int id_counter;
    int counter;
    int nthread;
    worker_t *list;
} pool_t;

int pool_manager(pool_t *pool);

pool_t *init_pool(int n);
void add_worker(pool_t *p);
int rem_worker(pool_t *p, pthread_t tid);

#endif