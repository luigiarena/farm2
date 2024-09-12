#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#include "masterworker.h"

pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pool_cond = PTHREAD_COND_INITIALIZER;

void MasterWorker(char *argv[], int argc, int optind, int nthread, int qlen, int tdelay, char *dname) {
    printf("Sono MasterWorker (PID: %d)\n", getpid());

    int control_master = 1;
    int server_socket;
    struct sockaddr_un sa;

    // Gestore segnali per il MasterWorker
    signal(SIGHUP, handler_signals);
    signal(SIGINT, handler_signals);
    signal(SIGQUIT, handler_signals);
    signal(SIGTERM, handler_signals);
    signal(SIGUSR1, handler_signals);
    signal(SIGUSR2, handler_signals);

	// Uso optind per gestire tutti gli argomenti che non sono stati riconosciuti come parametri
    // Qui riempio la coda concorrente, se il file è regolare lo inserisco altrimenti lo ignoro
	for(; optind < argc; optind++){      
        printf("extra arguments: %s\n", argv[optind]);  
    } 

    // Creazione socket
    server_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("MasterWorker error -> creazione socket fallita\n");
        exit(EXIT_FAILURE);
    }

        // Configurazione socket
        server_addr.sun_family = AF_UNIX;
        strcpy(server_addr.sun_path, SOCKET_PATH);

        // Connessione al Collector
        if (connect(server_socket, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
            perror("MasterWorker error -> connessione fallita");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        printf("Connessione con Collector stabilita...\n");

    // Rimanere attivo per testare i segnali
    /*
    int i = 0;
    while (++i < 5) {
        printf("MasterWorker in esecuzione...\n");
        sleep(1);
    }
    */
}

// Funzione per la gestione dei segnali in MasterWorker
void handler_signals(int sig_rec) {
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

// Funzione che esplora la directory, saltando file ., .. e nascosti
void naviga_dir(const char *dname) {
    struct dirent *entry;
    struct stat file_stat;

    DIR *dir = opendir(dname);
    if (!dir) {
        perror("Errore nell'aprire la directory dei file di input");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[1024];

        // Salta "." e ".." e i file nascosti
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Crea il path completo
        snprintf(full_path, sizeof(full_path), "%s/%s", dname, entry->d_name);

        // Ottieni informazioni sul file
        if (stat(full_path, &file_stat) == -1) {
            perror("Errore nell'ottenere informazioni sul file");
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            // Se è una directory la esplorala ricorsivamente
            //--printf("Directory: %s\n", full_path);
            naviga_dir(full_path);
        } else if (S_ISREG(file_stat.st_mode)) {
            // Se è un file regolare lo aggiungo alla coda concorrente
            printf("File regolare: %s\n", full_path);
        }
    }

    closedir(dir);
}
