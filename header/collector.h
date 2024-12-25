/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: collector.h
    Descrizione: 
*/

#define SOCKET_PATH			"./farm2.sck"
#define BUF_MAX_SIZE                  255
#define PATH_MAX_LEN                  255

// Struttura necessaria alla creazione di una lista per i risultati ricevuti da Collector
typedef struct result {
	long sum;
	char path[PATH_MAX_LEN];
	struct result *next;
} result_t;

void printlist(result_t *list);
void mask_signals_collector();
void cleanup();
void collector_main(int tdelay);
