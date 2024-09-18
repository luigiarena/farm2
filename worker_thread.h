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

void add_worker(Worker_list *l);

void rem_worker(Worker_list *l);

void print_nworkers(Worker_list *l);

void free_lista(Worker_list *l);

void free_nodo(Worker_node *w);