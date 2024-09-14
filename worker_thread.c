#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "worker_thread.h"

// Funzione eseguita da ogni worker thread
void* worker_thread(void* arg) {
    int id = *((int*)arg);
    id = pthread_self();
    printf("Worker %d: Inizio esecuzione (thread id: %lu)\n", id, pthread_self());
    while (1) {
        printf("Worker %d: Eseguendo...\n", id);
        sleep(5);
    }
    return NULL;
}