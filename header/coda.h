/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: coda.h
    Descrizione: 
*/

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

//----------------------------------------------

#include <utility.h>

typedef struct nodo {
    char *file_path;
    struct nodo *next;
} Nodo;

typedef struct coda {
    int len;
    int max;
    Nodo *head;
    Nodo *tail;
    pthread_mutex_t lock;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} Coda;

Coda *create_coda(int qlen);
void free_coda(Coda *q);
int push_coda(Coda *q, char *path);
char* pop_coda(Coda *q);
void stampa_coda(Coda *q);
