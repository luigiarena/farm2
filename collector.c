#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include "collector.h"

#define SOCKET_PATH			"./farm2.sck"

void Collector() {
    printf("Sono Collector (PID: %d)\n", getpid());

    // Maschera i segnali per il processo Collector
    mask_signals();

    cleanup();

    // Rimanere attivo per testare i segnali
    int i = 0;
    while (++i < 5) {
        printf("Collector in esecuzione\n");
        sleep(1);
    }
}

void mask_signals() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    /*
   if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        perror("Errore nel mascherare i segnali nel Collector");
        exit(EXIT_FAILURE);
    } */
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("Errore nel mascherare i segnali nel Collector");
        exit(EXIT_FAILURE);
    }
}

void cleanup() {
    unlink(SOCKET_PATH);
}