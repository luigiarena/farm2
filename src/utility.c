/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: utility.c
    Contiene la variabile verbose usata da tutti gli altri file e
    la funzine sleepTime che sfrutta nanosleep per mandare i thread in pausa
    usando i millisecondi
*/
#define _POSIX_C_SOURCE 200809L

#include <time.h> 
#include "utility.h"

//  Variabile globale utile per gestire lo stato verbose tra le opzioni del programma
int verbose;

/*
    Funzione per l'attesa in millisecondi, sfrutta nanosleep
    @param    milliseconds  intero lungo che indica la quantità di tempo in millisecondi
    @return   0             se ha successo
              -1            altrimenti
*/
int sleepTime(long miliseconds)
{
   struct timespec rem;
   struct timespec req = {
       (int)(miliseconds / 1000),      //  secs (Deve essere non negativo)
       (miliseconds % 1000) * 1000000  //  nano (Deve essere nel range tra 0 e 999999999)
   };

   return nanosleep(&req , &rem);
}