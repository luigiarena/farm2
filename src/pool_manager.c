/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: pool_manager.c
    Descrizione: 
*/

#include <stdio.h>

#include "utility.h"

extern int verbose;

void *poolManager(void *arg) {
    V_PRINT_MSG(MASTERWORKER, "sono il thread pool manager!");
    printf(MASTERWORKER " sono il thread pool manager!\n");
    return NULL;
}