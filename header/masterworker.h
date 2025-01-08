/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: masterworker.h
    Descrizione: 
*/
#ifndef MASTERWORKER_H
#define MASTERWORKER_H

/*
typedef struct worker_data {
    int worker_id;
    // questo void sarà castato per contenere un puntatore alla coda concorrente, qui non definita
    Coda *coda_link;
    // Serve?
    Worker_list *w_list_link;
} Worker_data;
*/

typedef struct master_data {
    int nthread;                // numero di threads
    int qlen;                   // lunghezza della coda concorrente
    long tdelay;                // tempo di ritardo nell'inserimento dei task
    char *dname;                // path della directory da visitare
    int num_file;               // numero di file inseriti come argomento
    char **file_list;           // lista dei nomi dei file inseriti
} master_data_t;

//static void *handler_signals(void *arg);
void masterWorker_main(master_data_t *data);

#endif