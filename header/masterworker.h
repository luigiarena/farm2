/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: masterworker.h
    Header del file masterworker. Descrive la struttura per il salvataggio dei
    dati necessari al funzionamento del programma e la funzione che farm usa
    per avviare masterworker - sono comunque lo stesso processo, al contrario
    di collector. Contiene anche la funzione per eliminare la struttura dei dati
*/
#ifndef MASTERWORKER_H
#define MASTERWORKER_H

typedef struct master_data {
    int nthread;                // numero di threads
    int qlen;                   // lunghezza della coda concorrente
    long tdelay;                // tempo di ritardo nell'inserimento dei task
    char *dname;                // path della directory da visitare
    int num_file;               // numero di file inseriti come argomento
    char **file_list;           // lista dei nomi dei file inseriti
} master_data_t;

void masterWorker_main(master_data_t *data);
void free_data(master_data_t *data);

#endif