/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: coda.c
    Descrizione: 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "coda.h"
#include "utility.h"
/*
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
*/
extern volatile sig_atomic_t no_more_files;

coda_t *init_coda(int size) {
    coda_t *c = malloc(sizeof(coda_t)+(size*sizeof(char *)));
    pthread_mutex_init(&c->mtx, NULL);
    pthread_cond_init(&c->full, NULL);
    pthread_cond_init(&c->empty, NULL);
    //c->mtx = &mtx;
    //c->full = &full;
    //c->empty = &empty;
    c->size = size;
    c->counter = 0;
    c->reader = 0;
    c->writer = 0;
    c->tot = 0;
    for (int i=0; i<size; i++) {
        c->task[i] = malloc(sizeof(char)*PATH_MAX_LEN);
    }
    return c;
}

void scrivi_coda(coda_t *c, char *path) {
    //printf("SCRIVI CODA cerca LOCK\n");
    pthread_mutex_lock(&c->mtx);
    //printf("SCRIVI CODA prende LOCK\n");
    //printf("Path ricevuto: %s\n", path);
    // Se la coda è piena aspetta che venga svuotata
    if (c->counter == c->size) pthread_cond_wait(&c->full, &c->mtx);
    strncpy(c->task[c->writer], path, PATH_MAX_LEN);
    //c->task[c->writer] = path;
    c->counter++;
    c->tot++;
    c->writer++;
    if (c->writer == c->size) c->writer = 0;
    //c->writer = (c->writer + 1) % c->size;
    // Risveglia i thread in attesa del riempimento della coda
    pthread_cond_signal(&c->empty);
    pthread_mutex_unlock(&c->mtx);
    //printf("SCRIVI CODA lascia LOCK\n");
}

char *leggi_coda(coda_t *c) {
    char *path = malloc(sizeof(char)*PATH_MAX_LEN);
    //printf("LEGGI CODA cerca LOCK\n");
    pthread_mutex_lock(&c->mtx);
    //printf("LEGGI CODA prende LOCK\n");
    // Se la coda è vuota aspetta che venga riempita
    if (c->counter == 0) pthread_cond_wait(&c->empty, &c->mtx);
    strncpy(path, c->task[c->reader], PATH_MAX_LEN);
    printf("ESTRATTO DALLA CODA: %s\n", path);
    //path = c->task[c->reader];
    c->counter--;
    c->reader++;
    if (c->reader == c->size) c->reader = 0;
    //c->reader = (c->reader + 1) % c->size;
    // Risveglia i thread in attesa dello svuotamento della coda
    pthread_cond_signal(&c->full);
    pthread_mutex_unlock(&c->mtx);
    //printf("LEGGI CODA lascia LOCK\n");
    return path;
}

void printf_coda(coda_t *c) {
    printf("Stampa contenuto della coda concorrente\n");
    pthread_mutex_lock(&c->mtx);
    for (int i=0; i<c->counter; i++) printf("File %d: %s\n", i+1, c->task[i]);
    pthread_mutex_lock(&c->mtx);
    return;
}
