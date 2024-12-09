#include <pthread.h>

#include "pool_manager.h"

Coda crea_coda() {
    Coda q;
    q.size = 0;
    q.head = NULL;
    q.tail = NULL;
    q.lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    return q;
}

void distruggi() {

}