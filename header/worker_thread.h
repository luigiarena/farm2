/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: worker_thread.h
    Descrizione: 
*/

#include <stdio.h>

typedef struct nodo_proc {
    char *file_path;
    long result;
    struct nodo *next;
} Nodo_proc;

typedef struct worker_node {
    pthread_t tid;
    struct worker_node *next;
} Worker_node;

typedef struct worker_list {
    int count_w;
    Worker_node *head;
} Worker_list;

/*
typedef struct worker_data {
    int worker_id;
    // questo void sarà castato per contenere un puntatore alla coda concorrente, qui non definita
    void *coda_link;
    // Serve?
    Worker_list *w_list_link;
} Worker_data;
*/

// Funzione eseguita da ogni worker thread
void* worker_thread(void* arg);

void mask_signals_worker();

long calcola_res (char *path_file);

void add_worker(Worker_list *l);

void rem_worker(Worker_list *l);

void save_nworkers(Worker_list *l, char *nworker_file);

void free_lista(Worker_list *l);

void free_nodo(Worker_node *w);