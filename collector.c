#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "collector.h"

#define SOCKET_PATH			"./farm2.sck"
#define BUF_MAX_SIZE                  255

void Collector() {

    int control = 1;

    char *buffer[BUF_MAX_SIZE];
    int server_socket, client_socket;
    struct sockaddr_un sa;

    printf("Sono Collector (PID: %d)\n", getpid());

    // Maschera i segnali per il processo Collector
    mask_signals();

    // Rimuove il vecchio socket se esiste
    cleanup();

    // Creazione socket
    server_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Collector error -> creazione socket fallita\n");
        exit(EXIT_FAILURE);
    }

    // Configurazione socket
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCKET_PATH);

    // Binding
    if (bind(server_socket, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        perror("Collector error -> bind connessione");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_socket, 1) < 0) {
        perror("Collector error -> listen connessione");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Collector in ascolto...\n");

    while (control) {
        // Accetta connessioni dai worker
        if (client_socket < 0) {
            perror("Collector error -> accept connessione");
            close(server_socket);
            exit(1);
        }
        // Ricezione del messaggio
        int msg_read = read(client_socket, buffer, sizeof(buffer) - 1);
        if (msg_read < 0) {
            fprintf(stderr, "Collector error -> read del messaggio, errno: %d\n", errno);
            close(client_socket);
            continue;
        }
        if (strcmp(s_args.file_path, "STOP") == 0) {
            // Invio la risposta al client
            char ack[256] = "ack";
            write(client_socket, ack, strlen(ack));
            control = 0;
        }

        buffer[msg_read] = '\0';  // Assicura la terminazione della stringa

        // Stampa il messaggio ricevuto
        printf("Collector received: %s", buffer);

        // Chiude la connessione con il client
        close(client_socket);
    }
    
    // Chiusura del socket server
    close(server_socket);
    unlink(SOCKET_PATH);
    printf("Collector terminato.\n");

    // Rimanere attivo per testare i segnali
    /*
    int i = 0;
    while (++i < 5) {
        printf("Collector in esecuzione\n");
        sleep(1);
    }
    */
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
        perror("Collector error -> maschera segnali");
        exit(EXIT_FAILURE);
    }
}

void cleanup() {
    unlink(SOCKET_PATH);
}