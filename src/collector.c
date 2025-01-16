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

//  Mutex necessario per l'accesso alla struttura dei risultati
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

//  Variabili globali per gestire la terminazione dei cicli di Collector e del suo thread figlio
volatile sig_atomic_t stop_collector = 0;
volatile sig_atomic_t stop_printer = 0;

//  Struttura contenente i risultati ricevuti dai worker
typedef struct result {
	long sum;
    char *path;
	struct result *next;
} result_t;

//  Dichiarazione globale di un puntatore alla lista dei risultati
result_t *result_list = NULL;

//  Firma della funzione lanciata dal thread printer
static void *printerThread (void *arg);

//  Firma delle funzioni utilizzate internamente da collector
void mask_signals_collector();
int add_res(long sum, char *path);
void free_res(result_t * result_list);
void printlist();

/*
    Funzione main di Collector
*/
void collector_main() {

    //  Collector stampa il proprio PID per segnalare il suo avvio corretto
    V_PRINT_ARG(COLLECTOR, "si avvia - PID: %d", getpid());

    //  Maschera i segnali
    mask_signals_collector();

    //  Variabili utili alla connessione server
    int server_socket, client_socket;
    struct sockaddr_un server_addr;
    char buffer[BUF_MAX_SIZE];
    int nread;

    //  Creazione socket
    server_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Collector error -> creazione socket fallita\n");
        exit(EXIT_FAILURE);
    }
    V_PRINT_MSG(COLLECTOR, "socket creato");

    //  Configurazione socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    //  Rimuove il vecchio socket se esiste
    unlink(SOCKET_PATH);
    
    //  Binding
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Collector error -> bind connessione");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    V_PRINT_MSG(COLLECTOR, "bind socket");

    //  Listen
    if (listen(server_socket, 1) == -1                          ) {
        perror("Collector error -> listen connessione");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    V_PRINT_MSG(COLLECTOR, "listen socket");

    //  Avvia il thread printer per la stampa parziale dei risultati
    pthread_t printer_tid;
    if (pthread_create(&printer_tid, NULL, printerThread, NULL) != 0) {
        perror("Collector -> errore durante la creazione di printer");
        exit(EXIT_FAILURE);
    }
    V_PRINT_MSG(COLLECTOR, "avvia thread printer");

    //  Accetta connessione
    client_socket = accept(server_socket, NULL, NULL);
    if (client_socket == -1) {
        perror("Collector error -> accept connessione");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    //  Collector entra in un loop di ascolto
    V_PRINT_MSG(COLLECTOR, "in ascolto sulla connessione socket");

    while (!stop_collector) {
        //  Resetta il buffer
        memset(buffer, 0, BUF_MAX_SIZE);
        //  Ricezione del messaggio
        nread = read(client_socket, buffer, sizeof(buffer) - 1);

        if (nread > 0) {
            //  Se il messaggio è stato ricevuto correttamente

            V_PRINT_MSG(COLLECTOR, "ha ricevuto un messaggio");
            //  Fa il parsing del messaggio ricevuto per salvare i valori ricevuti
            long res = atol(strtok(buffer, ":"));
            char *path = strtok(0, ":");

            //  Aggiunge i risultati alla lista
            add_res(res, path);
            V_PRINT_MSG(COLLECTOR, "ha aggiunto un nuovo risultato");

            //  Manda un messaggio di conferma al client in attesa di risposta
            char ack[4] = "ACK";
            if (send(client_socket, ack, 4, 0) == -1) {
                perror("Errore nell'invio del messaggio");
            }
            continue;
        } else if (nread == -1) {
            //  Se c'è stato un errore nella ricezione del messaggio

            fprintf(stderr, "Collector error -> read del messaggio, errno: %d\n", errno);
            continue;
        } else if (nread == 0) {
            //  Se è stato ricevuto un messaggio vuoto Collector termina

            V_PRINT_MSG(COLLECTOR, "ricevuta richiesta di terminazione");
            stop_collector = 1;
            stop_printer = 1;
            continue;
        }

    }

    //  Chiusura dei socket
    close(client_socket);
    close(server_socket);

    //  Esegue la join di printer
    if (pthread_join(printer_tid, NULL)) {
        perror("Collector -> errore join printer");
        exit(EXIT_FAILURE);
    }
    V_PRINT_MSG(COLLECTOR, "effettuata join con printer");

    V_PRINT_MSG(COLLECTOR, "stampa finale dei risultati");
    V_PRINT_TXT("-------------------------------------------");
    printlist();
    V_PRINT_TXT("-------------------------------------------");

    //  Libera lo spazio della lista dei risultati e cancella socket
    free_res(result_list);
    unlink(SOCKET_PATH);

    V_PRINT_MSG(COLLECTOR, "chiusura");

    return;
}

/*
    Maschera i segnali di collector
*/
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

/*
    Aggiunge un nuovo elemento alla lista dei risultati, rispettando l'ordine numerico dei long salvati
    @param    sum   valore calcolato sul file
              path  posizione del file
    @return   0     se il risultato è stato aggiunto correttamente
              -1    altrimenti
*/
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

/*
    Libera lo spazio allocato dalla lista dei risultati
*/
void free_res(result_t *result_list) {

    if (result_list == NULL) return;
    free_res(result_list->next);
    free(result_list->path);
    free(result_list);

    return;
}

/*
    Stampa lista dei risultati
*/
void printlist() {
	result_t *iter = result_list;

	while(iter != NULL) {
		fprintf(stdout, "%ld %s\n", iter->sum, iter->path);
		iter=iter->next;
	}
	fflush(stdout);
}

/*
    Funzione lanciata dal thread di stampa
    Stampa la lista dei risultati ogni secondo
*/
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