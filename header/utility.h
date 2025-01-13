/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: utility.h
    Header di Utility contiene tante macro utili alla stampa di debug
    e costanti comuni, oltre alla funzine sleepTime. 
    Utility è usato da quasi tutti i file
*/
#ifndef UTILITY_H
#define UTILITY_H

// Colori per la stampa su shell
#define ANSI_COLOR_GREY     "\x1b[30;1m"
#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_GREEN    "\x1b[32m"
#define ANSI_COLOR_YELLOW   "\x1b[33m"
#define ANSI_COLOR_BLUE     "\x1b[34m"
#define ANSI_COLOR_PURPLE   "\x1b[35m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_RESET    "\x1b[0m"

// Nomi per stampa con padding e colore
#define FARM                ANSI_COLOR_RED     "FARM        " ANSI_COLOR_RESET
#define MASTERWORKER        ANSI_COLOR_YELLOW  "MASTERWORKER" ANSI_COLOR_RESET
#define COLLECTOR           ANSI_COLOR_PURPLE  "COLLECTOR   " ANSI_COLOR_RESET
#define WORKER              ANSI_COLOR_CYAN    "WORKER      " ANSI_COLOR_RESET

// Macro per la stampa verbose (-v)
#define V_PRINT_TXT(text)               if(verbose)fprintf(stdout,text "\n");
#define V_PRINT_MSG(caller, msg)        if(verbose)fprintf(stdout,caller " -> " msg "\n");
#define V_PRINT_ARG(caller, msg,...)    if(verbose)fprintf(stdout,caller " -> " msg "\n",__VA_ARGS__);

// Macro per la cattura di eccezioni == val
#define ec_val(res,val,msg)  if((res)==val) {perror(msg); exit(EXIT_FAILURE);}
// Macro per la cattura di eccezioni != val
#define ec_not(res,val,msg)  if((res)!=val) {perror(msg); exit(EXIT_FAILURE);}

// Definizioni di alcune costanti comuni
#define SOCKET_PATH			"./farm2.sck"
#define BUF_MAX_SIZE                  265
#define PATH_MAX_LEN                  255

int sleepTime(long miliseconds);

#endif