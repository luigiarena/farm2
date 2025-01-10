/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: utility.c
    Descrizione: 
*/

#define _POSIX_C_SOURCE 200809L

#include <time.h> 
#include "utility.h"

int verbose;

// Funzione per l'attesa, strutta nanosleep
int sleepTime(long miliseconds)
{
   struct timespec rem;
   struct timespec req = {
       (int)(miliseconds / 1000),     /* secs (Must be Non-Negative) */ 
       (miliseconds % 1000) * 1000000 /* nano (Must be in range of 0 to 999999999) */ 
   };

   return nanosleep(&req , &rem);
}