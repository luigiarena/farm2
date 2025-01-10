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
extern volatile sig_atomic_t worker_control;

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
printf("PUSH 4\n");
    task_t *push = (task_t *)malloc(sizeof(task_t));
printf("PUSH 5\n");
    push->end = end;
    push->next = NULL;
    push->path = malloc(sizeof(char)*PATH_MAX_LEN);
    strncpy(push->path, path, PATH_MAX_LEN);
printf("PUSH 6\n");
    push->next = NULL;
printf("PUSH 7\n");
    if (coda->list == NULL) {
        printf("PUSH 8\n");
        coda->list = push;
    } else {
        printf("PUSH 9\n");
        task_t *temp = coda->list;
        printf("PUSH 10\n");
        while (temp->next != NULL) {
            temp = temp->next;
        }
        printf("PUSH 11\n");
        temp->next = push;
    }
printf("PUSH 12\n");
    coda->counter++;
    coda->tot++;
printf("PUSH 14\n");
    return;
}

char *pop_coda(coda_t *coda) {
printf("POP 1\n");
    if (coda->list == NULL) return NULL;
printf("POP 3\n");
    task_t *temp = coda->list;
    coda->list = coda->list->next;
    char *path = temp->path;
    free(temp);
    coda->counter--;

printf("POP 5\n");
    return path;
}


/*
void push_coda(coda_t *coda, char *path, int end) {
    printf("PUSH 1\n");
    pthread_mutex_lock(&coda->mtx);

    if (end) coda->end = 1;
    printf("PUSH 2\n");
    if (coda->counter == coda->size) {
        printf("PUSH 3\n");
        pthread_cond_wait(&coda->not_full, &coda->mtx);
    }
    printf("PUSH 4\n");
    task_t *push = (task_t *)malloc(sizeof(task_t));
    printf("PUSH 5\n");
    push->end = end;
    push->next = NULL;
    push->path = malloc(sizeof(char)*PATH_MAX_LEN);
    strncpy(push->path, path, PATH_MAX_LEN);
    printf("PUSH 6\n");
    push->next = NULL;
    printf("PUSH 7\n");
    if (coda->list == NULL) {
        printf("PUSH 8\n");
        coda->list = push;
    } else {
        printf("PUSH 9\n");
        task_t *temp = coda->list;
        printf("PUSH 10\n");
        while (temp->next != NULL) {
            temp = temp->next;
        }
        printf("PUSH 11\n");
        temp->next = push;
    }
    printf("PUSH 12\n");
    coda->counter++;
    coda->tot++;
    printf("PUSH 13\n");
    printf("Produtto ----------> %s\n", push->path);
    pthread_cond_signal(&coda->not_empty);
    pthread_mutex_unlock(&coda->mtx);
    printf("PUSH 14\n");
    return;
}
*/
/*
char *pop_coda(coda_t *coda) {
    printf("POP 1\n");
    pthread_mutex_lock(&coda->mtx);
    if (coda->counter == 0 && !coda->end) {
        printf("POP 1.1\n");
        pthread_cond_wait(&coda->not_empty, &coda->mtx);
    }
printf("POP 2\n");
    if (coda->list == NULL) return NULL;
printf("POP 3\n");
    task_t *temp = coda->list;
    coda->list = coda->list->next;
    char *path = temp->path;
    free(temp);
    coda->counter--;

printf("POP 4\n");
    printf("Consumato-------------------------------------> %s\n", path);
    pthread_cond_signal(&coda->not_full);
    pthread_mutex_unlock(&coda->mtx);
printf("POP 5\n");
    return path;
}
*/
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

void print_coda(coda_t *c) {
    task_t *iter = c->list;
    printf("Stampa contenuto della coda concorrente\n");
    pthread_mutex_lock(&c->mtx);
    int i = 0;
    while (iter != NULL) {
        printf("File %d: %s\n", i+1, iter->path);
        iter = iter->next;
        i++;
    }
    pthread_mutex_unlock(&c->mtx);
    return;
}
