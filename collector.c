#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include "collector.h"

void Collector() {
    printf("Sono Collector (PID: %d)\n", getpid());

    // Maschera i segnali per il processo Collector
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        perror("Errore nel mascherare i segnali nel Collector");
        exit(EXIT_FAILURE);
    }

    // Rimanere attivo per testare i segnali
    while (1) {
        printf("Collector in esecuzione\n");
        sleep(5);
    }
}
