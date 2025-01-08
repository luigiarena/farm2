/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: coda.h
    Descrizione: 
*/
#ifndef CODA_H
#define CODA_H

#include <pthread.h>

typedef struct {
    pthread_mutex_t mtx;
    pthread_cond_t full;
    pthread_cond_t empty;
    int size;
    int counter;
    int reader;
    int writer;
    int tot;
    char *task[];
} coda_t;

coda_t *init_coda(int size);
void scrivi_coda(coda_t *c, char *path);
char *leggi_coda(coda_t *c);
int printf_coda(coda_t *c);

#endif