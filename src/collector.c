/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: collector.c
    Descrizione: 
*/
#define _POSIX_C_SOURCE 200809L

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

extern int verbose;

// Mutex necessario per l'accesso alla struttura dei risultati
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

// Variabili globali per gestire la terminazione dei cicli di Collectore e del suo thread figlio
volatile sig_atomic_t stop_collector = 0;
volatile sig_atomic_t stop_printer = 0;

// Struttura contenente i risultati ricevuti da Collector
typedef struct result {
	long sum;
	//char path[PATH_MAX_LEN];
    char *path;
	struct result *next;
} result_t;

// Dichiarazione globale della lista dei risultati
result_t *result_list = NULL;

static void *printerThread (void *arg);

void mask_signals_collector();
int add_res(long sum, char *path);
void free_res(result_t * result_list);
void printlist();

/*
void conta_res() {
    int i=0;
    result_t *iter = result_list;
    while (iter != NULL) {
        iter = iter->next;
        i++;
    } 
    printf("Numero di res: %d\n", i);
    return;
}
*/

// Funzione main di Collector 
void collector_main() {
    // Maschera i segnali per il processo Collector
    mask_signals_collector();

    // Variabili utili alla connessione server
    char buffer[BUF_MAX_SIZE];

    int server_socket, client_socket;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    int nread;

    V_PRINT_ARG(COLLECTOR, "PID: %d", getpid());

    // Creazione socket
    server_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Collector error -> creazione socket fallita\n");
        exit(EXIT_FAILURE);
    } else V_PRINT_MSG(COLLECTOR, "socket creato");

    // Configurazione socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Rimuove il vecchio socket se esiste
    unlink(SOCKET_PATH);
    
    // Binding
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
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

    result_list = NULL;

    // Avvia il thread printer per la stampa parziale dei risultati
    pthread_t printer_tid;
    if (pthread_create(&printer_tid, NULL, printerThread, NULL) != 0) {
        perror("Collector -> errore durante la creazione di printer");
        exit(EXIT_FAILURE);
    }
    V_PRINT_MSG(COLLECTOR, "printer avviato");

    // Collector entra in un loop di ascolto
    V_PRINT_MSG(COLLECTOR, "In ascolto...");
    while (!stop_collector) {
        // Accetta connessioni
        client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("Collector error -> accept connessione");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // Resetta il buffer
        memset(buffer, 0, BUF_MAX_SIZE);
        // Ricezione del messaggio
        nread = read(client_socket, buffer, sizeof(buffer) - 1);
        if (nread < 0) {
            fprintf(stderr, "Collector error -> read del messaggio, errno: %d\n", errno);
            close(client_socket);
            continue;
        }

        // Assicura la terminazione della stringa
        buffer[nread] = '\0';  

        if (strcmp(buffer, "STOP") == 0) {
            // Invia la risposta al client
            stop_collector = 1;
            stop_printer = 1;

            V_PRINT_MSG(COLLECTOR, "ultima stampa dei risultati");
            printlist();
            V_PRINT_MSG(COLLECTOR, "invio ack a Masterworker per stop");
            char ack[256] = "ack";
            write(client_socket, ack, 4);

            // Esce dal ciclo
            close(client_socket);
            break;
        } 

        // Fa il parsing del messaggio ricevuto per salvare i valori ricevuti
        long res = atol(strtok(buffer, ":"));
        char *path = strtok(0, ":");

        // Aggiunge i risultati alla lista
        add_res(res, path);

        // Chiude la connessione con il client
        close(client_socket);
    }

    // Chiusura del socket server
    close(server_socket);
    V_PRINT_MSG(COLLECTOR, "chiusura");

    // Esegue la join di printer
    if (pthread_join(printer_tid, NULL)) {
        fprintf(stderr, "MasterWorker -> errore join explorer\n");
        exit(EXIT_FAILURE);
    }

    // Libera lo spazio della lista dei risultati
    free_res(result_list);

    exit(EXIT_SUCCESS);
}

// Maschera i segnali di collector
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

    // Ignora SIGPIPE
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

    new = malloc(sizeof(result_t));
    if(new == NULL) return -1;

    new->next = NULL;
    new->sum = sum;
    new->path = malloc(PATH_MAX_LEN);
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

// Libera lo spazio dedicato alla lista dei risultati
void free_res(result_t *result_list) {
    if (result_list == NULL) return;
    free_res(result_list->next);
    free(result_list->path);
    free(result_list);

    return;
}

// Stampa lista dei risultati
void printlist() {
	result_t *iter = result_list;

	while(iter != NULL) {
		fprintf(stdout, "%ld %s\n", iter->sum, iter->path);
		iter=iter->next;
	}
	fflush(stdout);
}

// Funzione lanciata dal thread di stampa
static void *printerThread (void *arg) {
    while(!stop_printer) {
        if(result_list != NULL) {
            pthread_mutex_lock(&result_mutex);
            printlist();
            pthread_mutex_unlock(&result_mutex);
        }
        sleepTime(1000);
    }
    pthread_exit(NULL);
}