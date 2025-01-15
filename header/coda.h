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
    char *path;                     //  stringa che contiene il path di un file
    struct task *next;              //  puntatore al task successivo
} task_t;

typedef struct coda {
    pthread_mutex_t mtx;            //  mutex della coda
    pthread_cond_t not_full;        //  var cond per l'attesa su coda piena
    pthread_cond_t not_empty;       //  var cond per l'attesa su coda vuota
    int size;                       //  lunghezza massima della coda
    int counter;                    //  contatore dei task della coda
    int tot;                        //  numero totale di task aggiunti
    task_t *list;                   //  puntatore alla lista dei task
} coda_t;

coda_t *init_coda(int size);
void push_coda(coda_t *coda, char *path);
char *pop_coda(coda_t *coda);
void free_coda(coda_t *coda);
void print_coda(coda_t *coda);

#endif