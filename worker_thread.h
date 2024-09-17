#include <stdio.h>

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

void add_worker();

void print_nworkers();

void free_lista();

void free_nodo(Worker_node *w);