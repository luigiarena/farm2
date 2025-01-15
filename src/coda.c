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

/*
    Inizializza la struttura della coda concorrente
    @param    size  Lunghezza massima della coda
    @return   coda  Puntatore alla coda creata
*/
coda_t *init_coda(int size) {
    coda_t *coda = malloc(sizeof(coda_t));
    ec_val(coda, NULL, "errore allocazione coda");
    pthread_mutex_init(&coda->mtx, NULL);
    pthread_cond_init(&coda->not_full, NULL);
    pthread_cond_init(&coda->not_empty, NULL);
    coda->size = size;
    coda->counter = 0;
    coda->tot = 0;
    coda->list = NULL;
    return coda;
}

/*
    Inserisce un task che contiene la stringa path in fondo alla coda, politica FIFO
    @param    coda  Puntatore alla coda
              path  Stringa da inserire nel task
*/
void push_coda(coda_t *coda, char *path) {
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

/*
    Estrae e restituisce il path del task in cima ad una coda, eliminandolo da essa
    @param    coda  Puntatore alla coda
    @return   path  La stringa contenente il path
              NULL  Se la coda è vuota
*/ 
char *pop_coda(coda_t *coda) {
    if (coda->list == NULL) return NULL;

    task_t *temp = coda->list;
    coda->list = coda->list->next;
    char *path = temp->path;
    //  Elimina lo spazio allocato per il task estratto
    free(temp);
    coda->counter--;

    return path;
}

/*
    Libera la memoria dedicata alla lista dei task
    @param    task  Puntatore alla lista dei task
*/
void free_task_list(task_t *task) {
    if (task == NULL) return;
    free_task_list(task->next);
    free(task->path);
    free(task);

    return;
}

/*
    Libera la memoria dedicata alla coda
    @param    coda  Puntatore alla coda
*/
void free_coda(coda_t *coda) {
    free_task_list(coda->list);
    pthread_mutex_destroy(&coda->mtx);
    pthread_cond_destroy(&coda->not_full);
    pthread_cond_destroy(&coda->not_empty);
    free(coda);

    return;
}

/*
    Stampa la stato attuale della coda
    @param    coda  Puntatore alla coda
*/
void print_coda(coda_t *coda) {
    task_t *iter = coda->list;
    fprintf(stdout, "Stampa contenuto della coda concorrente\n");

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
