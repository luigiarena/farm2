/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: coda.c
    Descrizione: 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "coda.h"

coda_t *init_coda(int size) {
    coda_t *c = malloc(sizeof(coda_t)+size);
    pthread_mutex_init(&c->mtx, NULL);
    pthread_cond_init(&c->full, NULL);
    pthread_cond_init(&c->empty, NULL);
    c->size = size;
    c->counter = 0;
    c->reader = 0;
    c->writer = 0;
    c->tot = 0;
    return c;
}

void scrivi_coda(coda_t *c, char *path) {
    pthread_mutex_lock(&c->mtx);
    // Se la coda è piena aspetta che venga svuotata
    if (c->counter == c->size) pthread_cond_wait(&c->full, &c->mtx);
    c->task[c->writer] = path;
    c->counter++;
    c->tot++;
    //c->writer++;
    c->writer = (c->writer + 1) % c->size;
    // Risveglia i thread in attesa del riempimento della coda
    pthread_cond_signal(&c->empty);
    pthread_mutex_unlock(&c->mtx);
}

char *leggi_coda(coda_t *c) {
    char *path;
    pthread_mutex_lock(&c->mtx);
    // Se la coda è vuota aspetta che venga riempita
    if (c->counter == 0) pthread_cond_wait(&c->empty, &c->mtx);
    path = c->task[c->reader];
    c->counter--;
    c->reader++;
    c->reader = (c->reader + 1) % c->size;
    // Risveglia i thread in attesa dello svuotamento della coda
    pthread_cond_signal(&c->full);
    pthread_mutex_unlock(&c->mtx);
    return path;
}

int printf_coda(coda_t *c) {
    printf("Stampa contenuto della coda concorrente\n");
    for (int i=0; i<c->counter; i++) printf("File %d: %s\n", i+1, c->task[i]);
    return c->tot;
}

//----------------------------------------------

Coda *create_coda(int qlen) {
    Coda *q = malloc(sizeof(Coda));
    q->len = 0;
    q->max = qlen;
    q->head = NULL;
    q->tail = NULL;
    q->lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    return q;
}

void free_coda(Coda *q) {
    Nodo *iter = malloc(sizeof(Nodo));
    if (q == NULL) return;
    while (q->head != NULL) {
        iter->next = q->head;
        q->head = q->head->next;
        free(iter->next);
    }
    free(q);
    free(iter);
    return;
}

int push_coda(Coda *q, char *path) {
    Nodo *new = malloc(sizeof(Nodo));

    new->file_path = malloc(BUF_MAX_SIZE);
    strncpy(new->file_path, path, strlen(path));
    new->file_path[strlen(path)+1] = '\0';
    new->next = NULL;

    if (q->head == NULL) 
        q->head = new;
    else if (q->tail == NULL) 
        q->head->next = q->tail = new;
    else { 
        q->tail->next = new;
        q->tail = q->tail->next;
    }

    return ++(q->len);
}

char *pop_coda(Coda *q) {
    char *path = malloc(BUF_MAX_SIZE);

    if (q->head == NULL) return NULL;
    else {
        strncpy(path, q->head->file_path, strlen(q->head->file_path));
        path[strlen(q->head->file_path)+1] = '\0';

        q->head = q->head->next;
        if (q->head == NULL) {
            q->tail = NULL;
        }   

        q->len--;
    }

    return path;
}

// Funzione di test
void stampa_coda(Coda *q) {
    Nodo *iter = q->head;
    int i = 0;
    printf("Stampa contenuto della coda concorrente\n");
    while (iter != NULL) {
        printf("File %d: %s\n", i, iter->file_path);
        i++;
        iter = iter->next;
    }

    return;
}