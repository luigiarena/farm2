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

// Mutex per la gestione condivisa della socket tra i thread
extern int server_socket;
extern pthread_mutex_t socket_mtx;

extern int verbose;

extern coda_t *coda;
extern pool_t *pool;

int find_id(pool_t *pool, pthread_t tid);

// Funzione eseguita da ogni worker thread
void* worker_thread(void* arg) {

    // Converte il suo argomento di input in un puntatore ad una struttura pool
    //pool_t *pool = (pool_t *) arg;

    // Converte il suo argomento in un int id;
    int id = *(int *) arg;

    // Chiede il suo Id thread
    pthread_t tid = pthread_self();
    //V_PRINT_ARG(WORKER, "(%ld) partito", tid);

    // Cerca il suo Id incrementale
    //int id = find_id(pool, tid);
    V_PRINT_ARG(WORKER, "(%d) partito", id);

    // Definisce le variabili per la connessione al server
    int client_socket = server_socket;
    //printf("Worker %d con socket: %d\n", id, client_socket);
    //struct sockaddr_un server_addr;
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
        if (strcmp(path, "") != 0) {

            // Calcola il risultato del file
            res = calc_res(path);

            // Invia messaggio a Collector
            pthread_mutex_lock(&socket_mtx);

            memset(message, 0, BUF_MAX_SIZE);
            snprintf(message, sizeof(message), "%ld:%s", res, path);
            if (send(client_socket, message, strlen(message)+1, 0) == -1) {
                perror("Errore nell'invio del messaggio");
                pthread_mutex_unlock(&socket_mtx);
            }
            V_PRINT_ARG(WORKER, "(%d) ha inviato %s", id, message);
            memset(message, 0, BUF_MAX_SIZE);
            read(server_socket, message, BUF_MAX_SIZE);
            V_PRINT_ARG(WORKER, "(%d) ha ricevuto %s", id, message);

            pthread_mutex_unlock(&socket_mtx);

            free(path);
        } else break;
    }

    // Il Worker termina il suo ciclo
    V_PRINT_ARG(WORKER, "(%d) terminato", id);
    
    pthread_exit(NULL);
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

// Trova l'id incrementale del worker
int find_id(pool_t *pool, pthread_t tid) {
    int id = 0;
    pthread_mutex_lock(&pool->mtx);
    worker_t *w = pool->list;
    while (w != NULL && w->tid != tid) w = w->next;
    if (w != NULL) id = w->id;
    pthread_mutex_unlock(&pool->mtx);
    return id;
}
