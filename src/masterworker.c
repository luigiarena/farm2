/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: masterworker.c
    Descrizione: 
*/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

//#include <dirent.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
//#include <sys/stat.h>
//#include <sys/types.h>
//#include <sys/wait.h>

#include "masterworker.h"
#include "explorer.h"
#include "worker_thread.h"
#include "coda.h"
#include "pool_manager.h"
#include "utility.h"

#define MAX_NCONN                      10

extern int verbose;

volatile sig_atomic_t stop_signal = 0;
volatile sig_atomic_t usr1_signal = 0;
volatile sig_atomic_t usr2_signal = 0;
volatile sig_atomic_t usr_counter = 0;
volatile sig_atomic_t no_more_files = 0;

pthread_mutex_t usr_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pool_cond = PTHREAD_COND_INITIALIZER;

coda_t *coda;

static void *handler_signals(void *arg);
void save_nworkers(int n, char *file);

//void fill_coda(coda_t *c, char *file_list[], int file_num, long tdelay, char *dname);
//void explore_dir(coda_t *c, long tdelay, char *dname);

void masterWorker_main(master_data_t *data) {
    V_PRINT_ARG(MASTERWORKER, "PID: %d", getpid());

	// Gestisco segnali per MasterWorker
	sigset_t mask;

    ec_val(sigemptyset(&mask), -1, "Masterworker sigemptyset mask");

    ec_val(sigaddset(&mask, SIGHUP), -1, "Masterworker sigaddset sighup");
    ec_val(sigaddset(&mask, SIGINT), -1, "Masterworker sigaddset sigint");
    ec_val(sigaddset(&mask, SIGQUIT), -1, "Masterworker sigaddset sigquit");
    ec_val(sigaddset(&mask, SIGTERM), -1, "Masterworker sigaddset sigterm");
    ec_val(sigaddset(&mask, SIGUSR1), -1, "Masterworker sigaddset sigusr1");
    ec_val(sigaddset(&mask, SIGUSR2), -1, "Masterworker sigaddset sigusr2");

    // Ignoro SIGPIPE
    struct sigaction saction;
    memset(&saction, 0, sizeof(saction));
    saction.sa_handler = SIG_IGN;
    ec_val(sigaction(SIGPIPE, &saction, NULL), -1, "Masterworker sigaction ignore");

    // Applico sigmask
    ec_not(pthread_sigmask(SIG_BLOCK, &mask, NULL), 0, "Masterworker set sigmask");

    // Creo un thread detached che gestisce i segnali
    pthread_t handlerThread;
    ec_not(pthread_create(&handlerThread, NULL, &handler_signals, &mask), 0, "Masterworker pthread_create");
    ec_not(pthread_detach(handlerThread), 0, "Masterworker pthread_detach");

    // Creazione socket
    int server_socket;
    //int n_workers = nthread;

    struct sockaddr_un sa;
    char buffer[BUF_MAX_SIZE];

    server_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("MasterWorker error -> creazione socket fallita\n");
        exit(EXIT_FAILURE);
    }

    // Configurazione socket
    sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path, SOCKET_PATH);

    V_PRINT_MSG(MASTERWORKER, "tentativo di connessione");
    // Connessione al server (collector)
    int tentativi=0;
    while (tentativi<MAX_NCONN && (connect(server_socket, (struct sockaddr *)&sa, sizeof(sa)) == -1)) {
        perror("MasterWorker error -> connessione fallita");
        close(server_socket);
        exit(EXIT_FAILURE);
        tentativi++;
        sleep(1);
    }

    V_PRINT_MSG(MASTERWORKER, "connessione con Collector stabilita!")

    // Crea la coda concorrente
    coda = init_coda(data->qlen);
    printf("Coda init\n");

    // Crea il pool dei Worker
    pool_t *pool = init_pool(data->nthread);
    printf("Pool init\n");

    // Avvia il thread che si occupererà di riempire la coda

    pthread_t explorer_tid;
    if (pthread_create(&explorer_tid, NULL, explorer, data) != 0) {
        perror("Masterworker -> errore durante la creazione di explorer");
        exit(EXIT_FAILURE);
    } else V_PRINT_MSG(MASTERWORKER, "pool manager avviato");
    V_PRINT_ARG(MASTERWORKER, "avviato pool manager: %ld\n", explorer_tid);

    //printf("Counter della coda: %d\n", coda->counter);
    /*
    while(!stop_signal) {
        printf("Masterworker aspetta fine\n");
        if (coda->counter == 0 && no_more_files) stop_signal = 1;
        //else printf("Letto: %s\n", leggi_coda(coda));
        sleepTime(500);
    }
    */
    printf("Test add_worker su pool\n");
    int active_workes = pool_manager(pool);

    V_PRINT_MSG(MASTERWORKER, "prima della join");
        
    // Aspetto la fine della coda concorrente

    // Attende la chiusura di Explorer
    printf("Masterworker cerca di joinare explorer: %ld\n", explorer_tid);
    if (pthread_join(explorer_tid, NULL)) {
        fprintf(stderr, "MasterWorker -> errore join explorer\n");
        exit(EXIT_FAILURE);
    }

    V_PRINT_MSG(MASTERWORKER, "dopo della join");

    // Invio messaggio "STOP"
    send(server_socket, "STOP", strlen("STOP"), 0);
    memset(buffer, 0, BUF_MAX_SIZE);
    read(server_socket, buffer, BUF_MAX_SIZE);
    V_PRINT_ARG(MASTERWORKER, "ha ricevuto %s", buffer);

    // Chiusura connessione e cancellazione del socket
    close(server_socket);
    unlink(SOCKET_PATH);

    // Salvo su file il numero di thread worker attivi
    printf("Numero di worker attivi: %d\n", pool->nthread);
    printf("Numero di active_workers: %d\n", active_workes);
    save_nworkers(active_workes, "nworkeratexit.txt");

    V_PRINT_MSG(MASTERWORKER, "chiusura");

    return;
}

// Funzione per la gestione dei segnali in MasterWorker
static void *handler_signals(void *arg) {

    int sig;
    while(!stop_signal) {
        if (sigwait((sigset_t *)arg, &sig) != 0) {
            perror("fatal error: sigwait");
            return NULL;
        }
        switch(sig) {
            case SIGHUP:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGHUP\n", 34);
                stop_signal = 1;
                break;
            case SIGINT:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGINT\n", 34);
                stop_signal = 1;
                break;
            case SIGQUIT:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGQUIT\n", 35);
                //stop_signal = 1;
                printf("usr_counter prima: %d\n", usr_counter);
                pthread_mutex_lock(&usr_counter_mutex);
                usr_counter++;
                pthread_mutex_unlock(&usr_counter_mutex);
                printf("usr_counter dopo: %d\n", usr_counter);
                break;
            case SIGTERM:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGTERM\n", 35);
                stop_signal = 1;
                break;
            case SIGUSR1:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGUSR1\n", 35);
                pthread_mutex_lock(&usr_counter_mutex);
                usr_counter++;
                pthread_mutex_unlock(&usr_counter_mutex);
                break;
            case SIGUSR2:
                if(verbose==1) write(1, "\nMasterWorker -> ricevuto SIGUSR2\n", 35);
                pthread_mutex_lock(&usr_counter_mutex);
                usr_counter++;
                pthread_mutex_unlock(&usr_counter_mutex);
                break;
            default:
                break;
        }
    }
    return NULL;
}

void save_nworkers(int n, char *file) {
    printf("Stampa su file numero di Thread Worker alla chiusura: %d\n", n);
    FILE *fp = fopen(file, "w");
    ec_val(fp, NULL, "Errore apertura file nworker");
    fprintf(fp, "%d\n", n);
    fclose(fp);

    return;
}

/*
// Funzione che esplora la directory, saltando file ., .. e nascosti
void fill_coda(coda_t *c, char *file_list[], int file_num, long tdelay, char *dname) {
    printf("Esplorazione iniziata\n");
    int index = 0;
    FILE *new_file;
    // Inserisce prima la lista dei file passati come argomenti
    while (index < file_num && !stop_signal) {
        printf("Tentativo di inserimento file: %s\n", file_list[index]);
        new_file = fopen(file_list[index], "rb");
        ec_val(new_file, NULL, "Errore apertura file");
        if (new_file == NULL) {
            fprintf(stderr, "Errore apertura file: %s\n", file_list[index]);
            index++;
            continue;
        }
        fclose(new_file);
        // Attendo il ritardo tdelay
        sleepTime(tdelay);
        scrivi_coda(c, file_list[index]);
        // TEST STAMPA CALCOLO
        //printf("Test calcolo %s: %ld\n", file_list[index], calcola_res(file_list[index]));
        index++;
    }
    // Poi esploro la directory se è stata passata
    if (dname != NULL) explore_dir(c, tdelay, dname);

    printf("Esplorazione finita\n");
    no_more_files = 1;
    return;
}
void explore_dir(coda_t *c, long tdelay, char *dname) {
    struct dirent *entry;
    struct stat file_stat;

    DIR *dir = opendir(dname);
    if (!dir) {
        fprintf(stderr, "Errore nell'aprire la directory dei file di input: %s\n", dname);
        return;
    }

    while (!stop_signal && (entry = readdir(dir)) != NULL) {
        char full_path[PATH_MAX_LEN];

        // Salta "." e ".." e i file nascosti
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Crea il path completo
        int path_len = strlen(dname)+strlen(entry->d_name) + 2;
        snprintf(full_path, path_len, "%s/%s", dname, entry->d_name);

        // Ottieni informazioni sul file
        if (stat(full_path, &file_stat) == -1) {
            fprintf(stderr, "Errore nell'ottenere informazioni sul file: %s\n", full_path);
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            // Se è una directory la esplora ricorsivamente
            explore_dir(c, tdelay, full_path);
        } else if (S_ISREG(file_stat.st_mode)) {
            // Se è un file regolare lo aggiungo alla coda concorrente
            // Attendo il ritardo tdelay
            sleepTime(tdelay);
            scrivi_coda(c, full_path);
            // TEST STAMPA CALCOLO
            printf("Test calcolo %s: %ld\n", full_path, calcola_res(full_path));
        }
    }

    closedir(dir);
}
*/