/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: pool_manager.c
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
    
    fill_coda(coda, data);

    //printf_coda(coda);

    printf("Esplorazione finita\n");

    no_more_files = 1;
    scrivi_coda(coda, "");
    //printf_coda(coda);
    
    pthread_exit(NULL);
}

// Funzione che esplora la directory, saltando file ., .. e nascosti
void fill_coda(coda_t *coda, master_data_t *data) {
    printf("Esplorazione iniziata\n");
    int index = 0;
    FILE *new_file;

    // Inserisce prima la lista dei file passati come argomenti
    while (!stop_signal && index < data->num_file) {
        printf("Tentativo di inserimento file: %s\n", data->file_list[index]);
        new_file = fopen(data->file_list[index], "rb");
        //ec_val(new_file, NULL, "Errore apertura file");
        if (new_file == NULL) {
            fprintf(stderr, "Errore apertura file: %s\n", data->file_list[index]);
            index++;
            continue;
        }
        fclose(new_file);
        // Attendo il ritardo tdelay
        sleepTime(data->tdelay);
        scrivi_coda(coda, data->file_list[index]);
        // TEST STAMPA CALCOLO
        //printf("Test calcolo %s: %ld\n", file_list[index], calcola_res(file_list[index]));
        index++;
    }
    // Poi esploro la directory se è stata passata
    if (!stop_signal && data->dname != NULL) explore_dir(coda, data->tdelay, data->dname);

    return;
}
void explore_dir(coda_t *coda, long tdelay, char *dname) {
    printf("ESPLORA DIR: %s\n", dname);
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

        printf("-> full path: %s\n", full_path);

        // Ottieni informazioni sul file
        if (stat(full_path, &file_stat) == -1) {
            fprintf(stderr, "Errore nell'ottenere informazioni sul file: %s\n", full_path);
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            // Se è una directory la esplora ricorsivamente
            explore_dir(coda, tdelay, full_path);
        } else if (S_ISREG(file_stat.st_mode)) {
            // Se è un file regolare lo aggiungo alla coda concorrente
            // Attendo il ritardo tdelay
            sleepTime(tdelay);
            scrivi_coda(coda, full_path);
            // TEST STAMPA CALCOLO
            //printf("Test calcolo %s: %ld\n", full_path, calcola_res(full_path));
        }
    }

    closedir(dir);
}