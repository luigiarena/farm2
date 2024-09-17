#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "worker_thread.h"

extern volatile sig_atomic_t stop_signal;
extern volatile sig_atomic_t usr1_signal;
extern volatile sig_atomic_t usr2_signal;

extern Worker_list lista;

// Funzione eseguita da ogni worker thread
void* worker_thread(void* arg) {
    int id = *((int*)arg);
    id = pthread_self();
    printf("Worker %d: Inizio esecuzione (thread id: %lu)\n", id, pthread_self());
    while (!stop_signal) {
        printf("Worker %d: Eseguendo...\n", id);
        printf("Stato USR1: %d - Stato USR2: %d\n", usr1_signal, usr2_signal);
        sleep(5);
    }
    return NULL;
}