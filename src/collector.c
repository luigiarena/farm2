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
#include <pthread.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "collector.h"
#include "utility.h"

#define SOCKET_PATH			"./farm2.sck"
#define BUF_MAX_SIZE                  255

pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

extern int verbose;

static int control_collector;
static int control_printer;

result_t *result_list = NULL;

static void *printerThread (void *arg);

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

    result_list = NULL;

    add_res(10, "ciao ciao");
    V_PRINT_MSG(COLLECTOR, "risultato aggiunto!");
    add_res(2, "ciao ciao");
    V_PRINT_MSG(COLLECTOR, "risultato aggiunto!");
    add_res(3, "ciao ciao");
    V_PRINT_MSG(COLLECTOR, "risultato aggiunto!");
    add_res(5, "ciao ciao");
    V_PRINT_MSG(COLLECTOR, "risultato aggiunto!");
    add_res(8, "ciao ciao");
    V_PRINT_MSG(COLLECTOR, "risultato aggiunto!");
    add_res(1, "ciao ciao");
    V_PRINT_MSG(COLLECTOR, "risultato aggiunto!");
    add_res(7, "ciao ciao");
    V_PRINT_MSG(COLLECTOR, "risultato aggiunto!");
    add_res(9, "ciao ciao");
    V_PRINT_MSG(COLLECTOR, "risultato aggiunto!");
    add_res(4, "ciao ciao");
    V_PRINT_MSG(COLLECTOR, "risultato aggiunto!");
    add_res(6, "ciao ciao");
    V_PRINT_MSG(COLLECTOR, "risultato aggiunto!");


    // Avvia il thread printer per la stampa parziale dei risultati
    control_printer = 1;

    pthread_t printerId;
    if (pthread_create(&printerId, NULL, printerThread, NULL) != 0) {
        perror("Collector -> errore durante la creazione di printer");
        control_printer = 0;
        control_collector = 0;
        exit(EXIT_FAILURE);
    } else V_PRINT_MSG(COLLECTOR, "printer avviato");

    // Collector entra in un loop di ascolto
    control_collector = 1;
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
            control_collector = 0;
            control_printer = 0;
            V_PRINT_MSG(COLLECTOR, "ultima stampa dei risultati");
            printlist();
            free_res(result_list);
            //sleep(3);
            V_PRINT_MSG(COLLECTOR, "invio ack a Masterworker per stop");
            char ack[256] = "ack";
            write(client_socket, ack, strlen(ack));
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

// Aggiunge un nuovo elemento alla lista dei risultati, rispettando l'ordine numerico dei sum
int add_res(long sum, char *path) {
    pthread_mutex_lock(&result_mutex);
    result_t *iter = result_list;
    result_t *new;
    int trovato = 0;

    new = malloc(sizeof(result_t));
    if(new == NULL) return -1;

    new->next = NULL;
    new->sum = sum;
    strncpy(new->path, path, PATH_MAX_LEN);

    if(iter == NULL) {
        result_list = new;
    } else if(iter->sum > sum) {
        new->next = result_list;
        result_list = new;
    } else {
        while(iter->next != NULL && iter->next->sum <= sum)
            iter = iter->next;

        if(iter->next != NULL) new->next = iter->next;
        iter->next = new;
    }
    pthread_mutex_unlock(&result_mutex);
    return 0;
}

void free_res(result_t *res) {
    if(res->next != NULL) free_res(res->next);
    free(res);
}

// Stampa lista dei risultati
void printlist() {
	result_t *iter = result_list;

	while(iter != NULL) {
        //printf("ciclo di stampa %d\n", i++);
		fprintf(stdout, "%10ld %s\n", iter->sum, iter->path);
		iter=iter->next;
	}
	fflush(stdout);
}

static void *printerThread (void *arg) {
    while(control_printer) {
        //printf("Test di stampa del printer %d\n", ++i);
        if(result_list != NULL) {
            pthread_mutex_lock(&result_mutex);
            printlist();
            pthread_mutex_unlock(&result_mutex);
        }
        sleepTime(1000);
    }
    return NULL;
}