/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: worker_thread.h
    Header che contiene l'intestazione della funzione usata dai worker thread,
    una per mascherare i segnali e una per eseguire il calcolo sui file
*/
#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

void* worker_thread(void* arg);

#endif