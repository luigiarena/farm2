/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: coda.h
    Header file per la gestione della coda, dichiara il tipo di struttura
    che contiene la coda e i suoi task, con le relative funzioni per gestirla
*/
#ifndef CODA_H
#define CODA_H

#include <pthread.h>
#include <string.h>

typedef struct task {
    char *path;
    struct task *next;
} task_t;

typedef struct coda {
    pthread_mutex_t mtx;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    int size;
    int counter;
    int tot;
    task_t *list;
} coda_t;

coda_t *init_coda(int size);
void push_coda(coda_t *coda, char *path);
char *pop_coda(coda_t *coda);
void free_coda(coda_t *coda);
void print_coda(coda_t *coda);

#endif