/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: worker_thread.c
    File sorgente che descrive il funzionamento dei worker thread. Si avviano,
    estraggono dati dalla coda, calcolano il risultati sui file e lo inviano
    a Collector tramite connessione socket. Terminano in maniera autonoma
    grazie al funzionamento della coda.
*/
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "worker_thread.h"
#include "coda.h"
#include "pool_manager.h"
#include "utility.h"

extern volatile sig_atomic_t stop_signal;
extern volatile sig_atomic_t usr2_signal;

extern volatile sig_atomic_t no_more_files;

extern pthread_mutex_t socket_mtx;

extern int verbose;

extern coda_t *coda;

// Funzione eseguita da ogni worker thread
void* worker_thread(void* arg) {
    // Maschera i segnali
    mask_signals_worker();

    // Converte il suo argomento di input in un puntatore ad una struttura pool
    pool_t *pool = (pool_t *) arg;

    // Chiede il sui Id thread
    pthread_t tid = pthread_self();
    V_PRINT_ARG(WORKER, "(%ld) partito", tid);

    // Definisce le variabili per la connessione al server
    int client_socket;
    struct sockaddr_un server_addr;
    char message[BUF_MAX_SIZE];

    // Inizia il suo ciclo
    char *path;
    long res = 0;
    while (1) {
        // Ogni Worker avverte la variazione del segnale usr2
        // E il primo che lo riceve lo gestisce, auto eliminandosi
        if (usr2_signal != 0) {
            if (rem_worker(pool, tid) == 0) {
                usr2_signal--;
                break;
            } else {
                usr2_signal = 0;
            }
        }

        // Effettua un controllo sulla quantità di task
        // e aspetta se è solo vuota la lista, altrimenti
        // se non ci sono più file in arrivo manda un
        // segnale di sblocco per chi è in attesa
        pthread_mutex_lock(&coda->mtx);
        while (coda->counter == 0) {
            if (no_more_files) {
                pthread_cond_signal(&coda->not_empty);
                pthread_mutex_unlock(&coda->mtx);
                break;
            }
            pthread_cond_wait(&coda->not_empty, &coda->mtx);
        }

        // Estrae il prossimo path dalla coda
        path = pop_coda(coda);

        if (path == NULL) {
//            pthread_mutex_unlock(&coda->mtx);
            break;
        }

        pthread_cond_signal(&coda->not_full);
        pthread_mutex_unlock(&coda->mtx);

        // Controlla che il path sia valido
        if (strcmp(path, "")!=0) {

            // Calcola il risultato del file
            res = calc_res(path);

            // Invia messaggio a Collector

            // Creazione del socket
            if ((client_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
                perror("Errore nella creazione del socket client");
                pthread_exit(NULL);
            }

            // Configurazione del socket
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sun_family = AF_UNIX;
            strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

            // Connessione al server
            if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
                perror("Errore nella connessione al server");
                close(client_socket);
                pthread_exit(NULL);
            }

            // Invio del messaggio
            memset(message, 0, BUF_MAX_SIZE);
            snprintf(message, sizeof(message), "%ld:%s", res, path);
            if (send(client_socket, message, strlen(message), 0) == -1) {
                perror("Errore nell'invio del messaggio");
                close(client_socket);
                pthread_exit(NULL);
            }

            close(client_socket);
        } else break;
    }

    // Il Worker termina il suo ciclo
    V_PRINT_ARG(WORKER, "(%ld) terminato", tid);
    
    pthread_exit(NULL);
}

// Maschera i segnali
void mask_signals_worker() {
    sigset_t set;
    ec_val(sigemptyset(&set), -1, "Worker sigemptyset mask");

    ec_val(sigaddset(&set, SIGHUP), -1, "Worker sigaddset sighup");
    ec_val(sigaddset(&set, SIGINT), -1, "Worker sigaddset sigint");
    ec_val(sigaddset(&set, SIGQUIT), -1, "Worker sigaddset sigquit");
    ec_val(sigaddset(&set, SIGTERM), -1, "Worker sigaddset sigterm");
    ec_val(sigaddset(&set, SIGUSR1), -1, "Worker sigaddset sigurs1");
    ec_val(sigaddset(&set, SIGUSR2), -1, "Worker sigaddset sigusr2");
    
    ec_not(pthread_sigmask(SIG_BLOCK, &set, NULL), 0, "Worker set sigmask");

    // Ignora SIGPIPE
    struct sigaction saction;
    memset(&saction, 0, sizeof(saction));
    saction.sa_handler = SIG_IGN;
    ec_val(sigaction(SIGPIPE, &saction, NULL), -1, "Worker sigaction ignore");
}

// Calcola un long, risultato della sommatoria dei numeri contenuti nel file, moltiplicati
// per la loro posizione di riga all'interno del stesso
long calc_res (char *path_file){
    FILE *fp;
    // Apre il file
    fp = fopen(path_file, "rb");
    if (fp == NULL) {
        printf("Errore nell'apertura del file %s\n", path_file);
        return -1;
    }

    // Posizionamento alla fine del file
    fseek(fp, 0L, SEEK_END);

    // Ottiene posizione corrente (che è la dimensione del file)
    long size = ftell(fp);
    size = size / sizeof(long);
    long vals[size];

    fseek(fp, 0L, SEEK_SET);
    // Legge i valori dal file e li inserisce nell'array
    int count = fread(vals, sizeof(long), size, fp);
    fclose(fp);
    if(count != size){
        // Non ha letto il numero corretto di elementi del file
        printf("Errore lettura long nel file: %s\n", path_file);
        exit(EXIT_FAILURE);
    }
    // Se la lettura è andata a buon fine esegue il calcolo di result
    long result = 0, i;
    for(i=0;i<count;i++){
        result+=(i*vals[i]);
    }
    return result;
}
