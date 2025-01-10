/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: explorer.c
    Descrizione: 
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "explorer.h"
#include "coda.h"
#include "masterworker.h"
#include "utility.h"

extern volatile sig_atomic_t stop_signal;
extern volatile sig_atomic_t no_more_files;

extern coda_t *coda;

void fill_coda(coda_t *coda, master_data_t *data);
void explore_dir(coda_t *coda, long tdelay, char *dname);

void *explorer (void *arg) {
    master_data_t *data = (master_data_t *) arg;
    
    // Riempie la coda
    fill_coda(coda, data);

    // Setta la variabile no_more_files
    no_more_files = 1;

    // Manda un segnale per sbloccare tutti i thread in attesa sulla coda vuota
    pthread_mutex_lock(&coda->mtx);
    pthread_cond_broadcast(&coda->not_empty);
    pthread_mutex_unlock(&coda->mtx);

    pthread_exit(NULL);
}

// Funzione che esplora la directory, saltando file ., .. e nascosti
void fill_coda(coda_t *coda, master_data_t *data) {
    //printf("Esplorazione iniziata\n");
    int index = 0;
    FILE *new_file;

    // Inserisce prima la lista dei file passati come argomenti
    while (!stop_signal && index < data->num_file) {
        new_file = fopen(data->file_list[index], "rb");
        if (new_file == NULL) {
            fprintf(stderr, "Errore apertura file: %s\n", data->file_list[index]);
            index++;
            continue;
        }
        fclose(new_file);

        // Attende il ritardo tdelay
        sleepTime(data->tdelay);

        // Inserisce la stringa nella coda
        pthread_mutex_lock(&coda->mtx);
        while (coda->counter == coda->size) {
            pthread_cond_wait(&coda->not_full, &coda->mtx);
        }

        push_coda(coda, data->file_list[index], 0);

        pthread_cond_signal(&coda->not_empty);
        pthread_mutex_unlock(&coda->mtx);

        index++;
    }

    // Esplora la directory se è stata passata
    if (!stop_signal && data->dname != NULL) explore_dir(coda, data->tdelay, data->dname);

    //printf("Esplorazione finita\n");
    return;
}
void explore_dir(coda_t *coda, long tdelay, char *dname) {
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

        // Ottiene informazioni sul file
        if (stat(full_path, &file_stat) == -1) {
            fprintf(stderr, "Errore nell'ottenere informazioni sul file: %s\n", full_path);
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            // Se è una directory la esplora ricorsivamente
            explore_dir(coda, tdelay, full_path);
        } else if (S_ISREG(file_stat.st_mode)) {
            // Se è un file regolare attende il ritardo tdelay e lo aggiunge alla coda
            sleepTime(tdelay);

            pthread_mutex_lock(&coda->mtx);
            while (coda->counter == coda->size) {
                pthread_cond_wait(&coda->not_full, &coda->mtx);
            }

            push_coda(coda, full_path, 0);

            pthread_cond_signal(&coda->not_empty);
            pthread_mutex_unlock(&coda->mtx);
        }
    }

    closedir(dir);
}