/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: farm.c
    Descrizione: 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <dirent.h>

#include <sys/socket.h>
#include <sys/un.h>
//#include <sys/types.h>
#include <sys/wait.h>
//#include <sys/stat.h>

#include "utility.h"
#include "masterworker.h"
#include "collector.h"

#define NTHREAD_MIN						1
#define NTHREAD_DEFAULT 				4
#define QLEN_DEFAULT					8
#define TDELAY_DEFAULT					0
#define DNAME_PATHLEN				  255

//#define len_verbose 20

void usage_help(char *pname);
int add_dir(const char *dname, char *ar[], int index);

struct sockaddr_un sa;
extern int verbose;

// Funzione main per che lancia MasterWorker e fa partire il programma
int main(int argc, char *argv[]){

	// Dichiarazione e definizione di default delle variabili
	pid_t pid;
	int nthread = NTHREAD_DEFAULT;		// numero di threads
	int qlen = QLEN_DEFAULT;			// lunghezza della coda concorrente
	long tdelay = TDELAY_DEFAULT;		// tempo di ritardo nell'inserimento dei task
	char *dname = NULL;		            // path della directory da visitare

    // Analizza i parametri dati in input
    verbose = 0;
	int opt;
    while ((opt = getopt(argc, argv, "hvn:q:d:t:")) != -1) {
        switch (opt) {
			case 'h':
    			printf("Opzioni:\n"
    				"\t-n nthread\tnumero iniziale di thread worker\t(default 1)\n"
    				"\t-q qlen\t\tlunghezza della coda concorrente\t(default 8)\n"
    				"\t-t tdelay\tritardo inserimento task nella coda\t(default 0)\n"
    				"\t-d dname\tnaviga nella directory per cercare\n"
						"\t\t\tfile da leggere in input\t\t(default .)\n");
    			exit(EXIT_SUCCESS);
            case 'v':
                verbose = 1;
                break;
            case 'n':
                nthread = atoi(optarg);
                if (nthread < NTHREAD_MIN) {
                    fprintf(stderr, "Il numero di thread deve essere un numero intero maggiore o uguale ad %d\n", NTHREAD_MIN);
                    exit(EXIT_FAILURE);
                }
                break;
			case 'q':
                qlen = atoi(optarg);
				if (qlen < 1) {
                    fprintf(stderr, "La lunghezza della cosa deve essere un numero intero maggiore o uguale ad 1\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 't':
                tdelay = atol(optarg);
	            if (tdelay < 0) {
                    fprintf(stderr, "Il tempo di delay deve essere un numero intero maggiore o uguale ad 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'd':
                dname = optarg;
                break;
            default:
            usage_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    V_PRINT_MSG(FARM, "apertura");

    // Se non esistono argomenti e -d non è settato chiude
	if (optind >= argc && dname == NULL) {
		fprintf(stderr, "Nessun argomento fornito al programma.\n");
        usage_help(argv[0]);
		exit(EXIT_FAILURE);
	}
    
    // Altrimenti inizializza un array dei file inseriti come argomenti
    int num_file = argc-optind;

    char *file_list[num_file];
    int index;
    for (index=0; index<num_file; index++) file_list[index] = malloc(PATH_MAX_LEN);

    index = 0;
    while (optind < argc) {
        strncpy(file_list[index], argv[optind], PATH_MAX_LEN);
        optind++;
        index++;
    }

    // TEST
    printf("Numero di argomenti: %d\n", num_file);

    for(int i=0; i<num_file; i++) printf("file n.%d: %s\n", i, file_list[i]);

	// Creazione del processo figlio
    V_PRINT_MSG(FARM, "creo processo collector");
	pid = fork();
	if (pid < 0) {
		perror("Errore nella creazione del secondo processo\n");
		exit(EXIT_FAILURE);
	}

	if(pid == 0) {
		// figlio: Collector

        V_PRINT_MSG(COLLECTOR, "avvio funzione main");
		collector_main(tdelay);
	} else {
		// padre: MasterWorker

        // Attendo che collector abbia avviato la connessione
		sleep(1);

        V_PRINT_MSG(FARM,"avvio processo main di masterworker");
		masterWorker_main(file_list, num_file, nthread, qlen, tdelay, dname);

        V_PRINT_MSG(MASTERWORKER,"attendo chiusura di collector");
        // Attendo la chiusura di Collector
		wait(NULL);

        V_PRINT_MSG(FARM,"chiusura");
	}

	return 0;
}

// Stampa un messaggio di aiuto sull'uso del programma
void usage_help(char* pname) {
    fprintf(stderr, "Usage: %s [-n nthread] [-q qlen] [-t tdelay] [-d dname] [-h] [file1 file2 ...]\n", pname);
}
