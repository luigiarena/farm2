#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "masterworker.h"

// Funzione per la gestione dei segnali in MasterWorker
void handle_signal(int sig_rec) {
    switch(sig_rec) {
        case SIGHUP:
            write(1, "MasterWorker: ricevuto SIGHUP\n", 31);
            break;
        case SIGINT:
            write(1, "MasterWorker: ricevuto SIGINT\n", 31);
            break;
        case SIGQUIT:
            write(1, "MasterWorker: ricevuto SIGQUIT\n", 32);
            break;
        case SIGTERM:
            write(1, "MasterWorker: ricevuto SIGTERM\n", 32);
            break;
        case SIGUSR1:
            write(1, "MasterWorker: ricevuto SIGUSR1\n", 32);
            break;
        case SIGUSR2:
            write(1, "MasterWorker: ricevuto SIGUSR2\n", 32);
            break;
        default:
            break;

    }
}

void MasterWorker() {
    printf("Sono MasterWorker (PID: %d)\n", getpid());

    // Gestore segnali per il MasterWorker
    signal(SIGHUP, handle_signal);
    signal(SIGINT, handle_signal);
    signal(SIGQUIT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGUSR1, handle_signal);
    signal(SIGUSR2, handle_signal);

    // Rimanere attivo per testare i segnali
    while (1) {
        printf("MasterWorker in esecuzione...\n");
        sleep(5);
    }
}
