/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: coda.c
    Contiene le funzioni per la gestione della coda concorrente
*/
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "coda.h"
#include "utility.h"

// Inizializza la struttura della coda concorrente e ne ritorna il puntatore
coda_t *init_coda(int size) {
    coda_t *coda = malloc(sizeof(coda_t));
    ec_val(coda, NULL, "errore allocazione coda");
    pthread_mutex_init(&coda->mtx, NULL);           // mutex della coda
    pthread_cond_init(&coda->not_full, NULL);       // var cond per l'attesa su coda piena
    pthread_cond_init(&coda->not_empty, NULL);      // var cond per l'attesa su coda vuota
    coda->size = size;                              // lunghezza massima della coda
    coda->counter = 0;                              // contatore dei task della coda
    coda->tot = 0;                                  // numero totale di task aggiunti
    coda->list = NULL;                              // puntatore alla lista dei task
    return coda;
}

// Inserisce un task che contiene path in fondo alla coda, politica FIFO
void push_coda(coda_t *coda, char *path) {
    //printf("push in azione\n");
    if (path == NULL) return;
    task_t *push = (task_t *)malloc(sizeof(task_t));
    ec_val(push, NULL, "errore allocazione task");

    push->next = NULL;
    push->path = strndup(path, PATH_MAX_LEN);
    if (coda->list == NULL) {
        coda->list = push;
    } else {
        task_t *temp = coda->list;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = push;
    }
    coda->counter++;
    coda->tot++;

    return;
}

// Estrae il path del task in cima alla coda, eliminandolo da essa
char *pop_coda(coda_t *coda) {
    if (coda->list == NULL) return NULL;

    task_t *temp = coda->list;
    coda->list = coda->list->next;
    char *path = temp->path;
    free(temp);
    coda->counter--;

    return path;
}

// Libera la memoria dedicata alla lista dei task
void free_task_list(task_t *task) {
    printf("Pulizia task list -> null\n");
    if (task == NULL) return;
    else {
        //while (task->next != NULL) 
        printf("Pulizia task list -> iterazione\n");
        free_task_list(task->next);
        free(task->path);
        free(task);
    }
    return;
}

// Libera la memoria dedicata alla coda
void free_coda(coda_t *coda) {
    printf("Pulizia coda\n");
    free_task_list(coda->list);
    pthread_mutex_destroy(&coda->mtx);
    pthread_cond_destroy(&coda->not_full);
    pthread_cond_destroy(&coda->not_empty);
    free(coda);
    return;
}

// Stampa la stato attuale della coda
void print_coda(coda_t *coda) {
    task_t *iter = coda->list;
    printf("Stampa contenuto della coda concorrente\n");
    pthread_mutex_lock(&coda->mtx);
    int i = 0;
    while (iter != NULL) {
        printf("File %d: %s\n", i+1, iter->path);
        iter = iter->next;
        i++;
    }
    pthread_mutex_unlock(&coda->mtx);
    return;
}
