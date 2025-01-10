/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: worker_thread.c
    Descrizione: 
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

extern volatile sig_atomic_t worker_control;
extern volatile sig_atomic_t no_more_files;

extern pthread_mutex_t socket_mtx;

extern int verbose;

extern coda_t *coda;

//extern int no_more_files;
int trova_id(pool_t *p, pthread_t tid);
//void send_message(int socket, struct sockaddr_un *sa, pthread_t tid, char message[]);

// Funzione eseguita da ogni worker thread
void* worker_thread(void* arg) {
    mask_signals_worker();

    pool_t *pool = (pool_t *) arg;
    //printf("Test pool->counter: %d\n", pool->counter);
    //printf("Test pool->next->id: %d\n", pool->list->id);

    pthread_t tid = pthread_self();
    //int id = 0;
    //int id = trova_id(p, tid);
    //ec_val(id, 0, "Errore recupero id worker");

    V_PRINT_ARG(WORKER, "(%ld) avviato", tid);

    // Connessione al server
    int client_socket;
    struct sockaddr_un server_addr;
    char message[BUF_MAX_SIZE];

    char *path = malloc(PATH_MAX_LEN);
    long res = 0;
    while (!worker_control) {
        if (usr2_signal != 0) {
            if (rem_worker(pool, tid) == 0) {
                usr2_signal--;
                break;
            } else {
                usr2_signal = 0;
            }
        }
        //free(path);
        //sleepTime(200);
        //if (no_more_files && coda->counter == 0) break;
        /*
        if (coda->list == NULL) {
            continue;
        } else if (coda->list->end) {
            break;
        }
*/
printf("PRIMA DELLA LOCK\n");
        pthread_mutex_lock(&coda->mtx);
printf("DOPO LA LOCK\n");
        while (coda->counter == 0) {
            if (no_more_files) {
                pthread_cond_signal(&coda->not_empty);
                pthread_mutex_unlock(&coda->mtx);
                break;
            }
            printf("NEL WAIT\n");
            pthread_cond_wait(&coda->not_empty, &coda->mtx);
        }

        path = pop_coda(coda);
/*
        if (strcmp(path, "END") == 0) {
            push_coda(coda, "END", 1);
            break;
        }
*/
        if (path == NULL) {
            printf("CASO SPECIALE\n");
            pthread_mutex_unlock(&coda->mtx);
            break;
            /*
            pthread_mutex_lock(&coda->mtx);
            if (coda->end) {
                //push_coda(coda, "END", 1);
                printf("CASO SPECIALE END\n");
                pthread_cond_broadcast(&coda->not_empty);
                pthread_mutex_unlock(&coda->mtx);
                break;
            } else {
                printf("CASO SPECIALE NON END\n");
                pthread_mutex_unlock(&coda->mtx);
                continue;
            }
            */
        }

        pthread_cond_signal(&coda->not_full);
        pthread_mutex_unlock(&coda->mtx);
    printf("INTERMEZZO: %s\n", path);
/*
        if (strcmp(path, "")==0) {
            if (no_more_files && coda->list == NULL) push_coda(coda, "", 1);
            else continue;
        }
        */
        if (strcmp(path, "")!=0 && strcmp(path, "END")!=0) {
            //path = leggi_coda(coda);
            /*
            if (strcmp(path, "__NO_MORE_FILES__") == 0) {
                printf("WORKER CONTROL STOP\n");
                push_coda(coda, "__NO_MORE_FILES__", 1);
                //scrivi_coda(coda, "__NO_MORE_FILES__");
                //worker_control = 1;
                break;
            } else if (strcmp(path, "") == 0) {
                continue;
            }
            */
            // Termino il ciclo se la coda è vuota;
            /*
            if (strcmp(path, "") == 0) {
                //---scrivi_coda(coda, "");
                break;
            }
            */
            res = calc_res(path);
            // Invio messaggio a Collector
            //pthread_mutex_lock(&socket_mtx);

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

            printf("Thread %ld ha inviato: %s\n", tid, message);
            close(client_socket);

            //pthread_mutex_unlock(&socket_mtx);

            // TEST DI STAMPA
            // printf("Worker %ld legge------->: %s\n", tid, path);
            //printf("%ld  %s\n", res, path);
        } else break;
    }

    V_PRINT_ARG(WORKER, "(%ld) terminato", tid);
    printf("Worker %ld terminato\n", tid);
    
    pthread_exit(NULL);
}

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

int trova_id(pool_t *p, pthread_t tid) {
    worker_t *w = malloc(sizeof(worker_t));
    int id = 0;
    printf("TROVA_ID cerca LOCK\n");
    pthread_mutex_lock(&p->mtx);
    printf("TROVA_ID prende LOCK\n");
/*    w = p->list;
    while (w != NULL && w->tid != tid) w = w->next;
*/    if (w != NULL) id = w->id;
    pthread_mutex_unlock(&p->mtx);
    printf("TROVA_ID rilascia LOCK\n");
    return id;
}