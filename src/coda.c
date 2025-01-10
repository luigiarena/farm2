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

#include "coda.h"
#include "utility.h"

coda_t *init_coda(int size) {
    coda_t *c = malloc(sizeof(coda_t));
    pthread_mutex_init(&c->mtx, NULL);
    pthread_cond_init(&c->not_full, NULL);
    pthread_cond_init(&c->not_empty, NULL);
    c->end = 0;
    c->size = size;
    c->counter = 0;
    c->tot = 0;
    c->list = NULL;
    return c;
}


void push_coda(coda_t *coda, char *path, int end) {
    task_t *push = (task_t *)malloc(sizeof(task_t));
    push->end = end;
    push->next = NULL;
    push->path = malloc(sizeof(char)*PATH_MAX_LEN);
    strncpy(push->path, path, PATH_MAX_LEN);
//printf("PUSH 6\n");
    push->next = NULL;
    if (coda->list == NULL) {
        coda->list = push;
    } else {
        task_t *temp = coda->list;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = push;
    }
//printf("PUSH 12\n");
    coda->counter++;
    coda->tot++;
//printf("PUSH 14\n");
    return;
}

char *pop_coda(coda_t *coda) {
//printf("POP 1\n");
    if (coda->list == NULL) return NULL;
//printf("POP 3\n");
    task_t *temp = coda->list;
    coda->list = coda->list->next;
    char *path = temp->path;
    free(temp);
    coda->counter--;

//printf("POP 5\n");
    return path;
}

void free_coda(coda_t *coda) {
    while (coda->list != NULL) {
        task_t *temp = coda->list;
        coda->list = coda->list->next;
        free(temp->path);
        free(temp);
    }
    pthread_mutex_destroy(&coda->mtx);
    pthread_cond_destroy(&coda->not_full);
    pthread_cond_destroy(&coda->not_empty);
    free(coda);
}

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
