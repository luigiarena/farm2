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

// Funzione eseguita da ogni worker thread
void* worker_thread(void* arg);

void mask_signals_worker();

long calcola_res (char *path_file);

void add_worker(Worker_list *l);

void rem_worker(Worker_list *l);

void print_nworkers(Worker_list *l);

void free_lista(Worker_list *l);

void free_nodo(Worker_node *w);