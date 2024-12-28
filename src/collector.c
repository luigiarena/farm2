/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: collector.c
    Descrizione: 
*/

//#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "collector.h"
#include "utility.h"

#define SOCKET_PATH			"./farm2.sck"
#define BUF_MAX_SIZE                  255

extern int verbose;

void collector_main(int tdelay) {

    char buffer[BUF_MAX_SIZE];
    int server_socket, client_socket;
    struct sockaddr_un sa;
    int nread;

    V_PRINT_ARG(COLLECTOR, "PID: %d", getpid());

    // Maschera i segnali per il processo Collector
    mask_signals_collector();

    // Rimuove il vecchio socket se esiste
    unlink(SOCKET_PATH);

    // Creazione socket
    server_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("COLLECTOR ERROR -> creazione socket fallita\n");
        exit(EXIT_FAILURE);
    } else V_PRINT_MSG(COLLECTOR, "socket creato");

    // Configurazione socket
    sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path, SOCKET_PATH);

    // Binding
    if (bind(server_socket, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        perror("Collector error -> bind connessione");
        close(server_socket);
        exit(EXIT_FAILURE);
    } else V_PRINT_MSG(COLLECTOR, "bind socket");

    // Listen
    if (listen(server_socket, 1) == -1                          ) {
        perror("Collector error -> listen connessione");
        close(server_socket);
        exit(EXIT_FAILURE);
    } else V_PRINT_MSG(COLLECTOR, "listen socket");

    V_PRINT_MSG(COLLECTOR, "In ascolto...");

    // Collector entra in un loop di ascolto
    int control_collector = 1;
    while (control_collector) {
        // Accetta connessioni
        client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0) {
            perror("Collector error -> accept connessione");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // Ricezione del messaggio
        nread = read(client_socket, buffer, sizeof(buffer) - 1);
        if (nread < 0) {
            fprintf(stderr, "Collector error -> read del messaggio, errno: %d\n", errno);
            close(client_socket);
            continue;
            //break;
        }

        buffer[nread] = '\0';  // Assicura la terminazione della stringa

        if (strcmp(buffer, "STOP") == 0) {
            // Invio la risposta al client
            V_PRINT_MSG(COLLECTOR, "invio ack per stop");
            char ack[256] = "ack";
            write(client_socket, ack, strlen(ack));
            control_collector = 0;
        }

        // Stampa il messaggio ricevuto
        V_PRINT_ARG(COLLECTOR, "ricevuto: %s", buffer);

        // Chiude la connessione con il client
        close(client_socket);
    }

    // Chiusura del socket server
    close(server_socket);
    unlink(SOCKET_PATH);
    V_PRINT_MSG(COLLECTOR, "chiusura");

    // Rimanere attivo per testare i segnali
    /*
    int i = 0;
    while (++i < 5) {
        printf("Collector in esecuzione\n");
        sleep(1);
    }
    */
}

void mask_signals_collector() {
    sigset_t set;
    ec_val(sigemptyset(&set), -1, "Collector sigemptyset mask");

    ec_val(sigaddset(&set, SIGHUP), -1, "Collector sigaddset sighup");
    ec_val(sigaddset(&set, SIGINT), -1, "Collector sigaddset sigint");
    ec_val(sigaddset(&set, SIGQUIT), -1, "Collector sigaddset sigquit");
    ec_val(sigaddset(&set, SIGTERM), -1, "Collector sigaddset sigterm");
    ec_val(sigaddset(&set, SIGUSR1), -1, "Collector sigaddset sigurs1");
    ec_val(sigaddset(&set, SIGUSR2), -1, "Collector sigaddset sigusr2");
    
    ec_not(pthread_sigmask(SIG_BLOCK, &set, NULL), 0, "Collector set sigmask");

    // Ignoro SIGPIPE
    struct sigaction saction;
    memset(&saction, 0, sizeof(saction));
    saction.sa_handler = SIG_IGN;
    ec_val(sigaction(SIGPIPE, &saction, NULL), -1, "Collector sigaction ignore");

}

// Stampa lista dei risultati
void printlist(result_t *lista_res) {
	result_t *iter = lista_res;
	while(iter != NULL) {
		fprintf(stdout, "%ld %s\n", iter->sum, iter->path);
		iter=iter->next;
	}
	fflush(stdout);
}