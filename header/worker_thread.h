/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: worker_thread.h
    Descrizione: 
*/
#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

// Funzione eseguita da ogni worker thread
void* worker_thread(void* arg);

void mask_signals_worker();

long calcola_res (char *path_file);

#endif