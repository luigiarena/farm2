/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: coda.h
    Descrizione: 
*/
#ifndef CODA_H
#define CODA_H

#include <pthread.h>

typedef struct task {
    char *path;
    int end;
    struct task *next;
} task_t;

typedef struct coda {
    pthread_mutex_t mtx;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    int end;
    int size;
    int counter;
    int tot;
    task_t *list;
} coda_t;

coda_t *init_coda(int size);
void push_coda(coda_t *c, char *path, int end);
char *pop_coda(coda_t *c);
void free_coda(coda_t *c);
void print_coda(coda_t *c);

#endif