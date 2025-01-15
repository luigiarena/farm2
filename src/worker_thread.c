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

extern int verbose;

extern volatile sig_atomic_t stop_signal;
extern volatile sig_atomic_t usr2_signal;

extern volatile sig_atomic_t no_more_files;

extern int server_socket;
extern pthread_mutex_t socket_mtx;

extern coda_t *coda;
extern pool_t *pool;

//  Firma delle funzioni utilizzate internamente da un worker thread
long calc_res (char *path_file);

/*
    Funzione eseguita da ogni worker thread, chiamata da pool_manager
    @param    arg  puntatore void castato ad un intero, id progressivo del worker
*/
void* worker_thread(void* arg) {

    //  Converte il suo argomento in un int id;
    int id = *(int *) arg;

    //  Chiede il suo Id thread
    pthread_t tid = pthread_self();

    V_PRINT_ARG(WORKER, "(%d) partito", id);

    //  Definisce le variabili utili per la connessione al server
    char message[BUF_MAX_SIZE];

    //  Inizia il suo ciclo
    char *path;
    long res = 0;
    while (1) {
        //  Ogni Worker avverte la variazione del segnale usr2
        //  e il primo che lo riceve lo gestisce, auto eliminandosi
        if (usr2_signal != 0) {
            if (rem_worker(pool, tid) == 0) {
                usr2_signal--;
                break;
            } else {
                usr2_signal = 0;
            }
        }

        //  Effettua un controllo sulla quantità di task e aspetta se la coda
        //  risulta vuota. Se però explorer ha anche mandato il segnale che 
        //  indica che non ci sono più file da aggiungere, allora il worker
        //  manda un segnale agli altri worker in attesa e sblocca la coda
        pthread_mutex_lock(&coda->mtx);
        while (coda->counter == 0) {
            if (no_more_files) {
                pthread_cond_signal(&coda->not_empty);
                pthread_mutex_unlock(&coda->mtx);
                break;
            }
            pthread_cond_wait(&coda->not_empty, &coda->mtx);
        }

        //  Estrae il prossimo path dalla coda
        path = pop_coda(coda);

        //  Se il path è NULL esce dal ciclo
        if (path == NULL) break;

        pthread_cond_signal(&coda->not_full);
        pthread_mutex_unlock(&coda->mtx);

        //  Controlla che il path sia valido altrimenti esce dal ciclo
        if (strcmp(path, "") != 0) {

            //  Calcola il risultato del file
            res = calc_res(path);
            if (res == -1) continue;

            //  Invia messaggio a Collector
            pthread_mutex_lock(&socket_mtx);

            memset(message, 0, BUF_MAX_SIZE);
            snprintf(message, sizeof(message), "%ld:%s", res, path);
            if (send(server_socket, message, strlen(message)+1, 0) == -1) {
                perror("Errore nell'invio del messaggio");
                pthread_mutex_unlock(&socket_mtx);
            }
            V_PRINT_ARG(WORKER, "(%d) ha inviato %s", id, message);
            memset(message, 0, BUF_MAX_SIZE);
            read(server_socket, message, BUF_MAX_SIZE);
            V_PRINT_ARG(WORKER, "(%d) ha ricevuto conferma %s", id, message);

            pthread_mutex_unlock(&socket_mtx);

            free(path);
        } else break;
    }

    //  Il Worker termina correttamente
    V_PRINT_ARG(WORKER, "(%d) terminato", id);
    
    pthread_exit(NULL);
}

/*
    Calcola un long, risultato della sommatoria dei numeri contenuti nel file, moltiplicati
    per la loro posizione all'interno dello stesso
    @param    path    posizione del file da elaborare
    @return   result  long risultato della sommatoria
              -1      se non è stato possibile effettuare il calcolo
*/
long calc_res (char *path){

    long result = 0;

    FILE *fp;
    //  Apre il file
    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Errore nell'apertura del file %s\n", path);
        return -1;
    }

    //  Calcola la dimensione in byte del file e del numero di long memorizzati
    //  creando un array con lo spazio necessario per memorizzarli
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    size = size / sizeof(long);
    long vals[size];

    //  Riempie l'array con i valori long letti dal file
    fseek(fp, 0L, SEEK_SET);
    int count = fread(vals, sizeof(long), size, fp);
    fclose(fp);

    if(count != size){
        //  Non ha letto il numero corretto di elementi del file
        printf("Errore lettura long nel file: %s\n", path);
        exit(EXIT_FAILURE);
    }

    //  Se la lettura è andata a buon fine esegue il calcolo di result
    long i;
    for(i=0;i<count;i++){
        result+=(i*vals[i]);
    }

    return result;
}