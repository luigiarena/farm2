/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: pool_manager.c
    Descrizione: 
*/

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "coda.h"

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
            //c->not_empty = NULL; //test
        }   

        q->len--;
    }

    return path;
}