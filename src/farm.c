/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: farm.c
    Contiene la funzione main del programma, esegue la scansione
    degli argomenti e delle opzioni ricevute in input facendo 
    poi partire sia masterworker che collector
*/
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <sys/wait.h>

#include "utility.h"
#include "explorer.h"
#include "masterworker.h"
#include "collector.h"

#define NTHREAD_MIN						1
#define NTHREAD_DEFAULT 				4
#define QLEN_DEFAULT					8
#define TDELAY_DEFAULT					0
#define DNAME_PATHLEN				  255

extern int verbose;

master_data_t *read_opt(int argc, char *argv[]);
void usage_help(char *pname);

// Funzione main del programma
int main(int argc, char *argv[]){

    verbose = 0;

    // Analizza i parametri dati in input
    master_data_t *data = malloc(sizeof(master_data_t));
    
    if (data == NULL) {
        fprintf(stderr, "Errore allocazione master data");
        exit(EXIT_FAILURE);
    }

    // Inizializza i valori della struttura di Masterworker
    data->nthread = NTHREAD_DEFAULT;
    data->qlen = QLEN_DEFAULT;
    data->tdelay = TDELAY_DEFAULT;
    data->num_file = 0;

    int dir_check = 0;
    int exit_check = 0;

    int opt;
    while ((opt = getopt(argc, argv, ":hvn:q:t:d:")) != -1) {
        switch (opt) {
            case 'h':
                printf("Opzioni:\n"
                    "\t-h            stampa questo messaggio informativo\n"
                    "\t-v            modalità verbose per seguire il flusso di lavoro\n"
                    "\t-n nthread    numero iniziale di thread worker     (default 1)\n"
                    "\t-q qlen       lunghezza della coda concorrente     (default 8)\n"
                    "\t-t tdelay     ritardo inserimento task nella coda  (default 0)\n"
                    "\t-d dname      naviga nella directory per cercare\n"
                    "\t              file da leggere in input             (default .)\n");
                exit_check = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'n':
                data->nthread = atoi(optarg);
                if (data->nthread < 1) {
                    fprintf(stderr, "Il numero di thread deve essere un numero intero maggiore o uguale ad 1\n");
                    exit_check = 1;
                }
                break;
            case 'q':
                data->qlen = atoi(optarg);
                if (data->qlen < 1) {
                    fprintf(stderr, "La lunghezza della coda deve essere un numero intero maggiore o uguale ad 1\n");
                    exit_check = 1;
                }
                break;
            case 't':
                data->tdelay = atol(optarg);
                if (data->tdelay < 0) {
                    fprintf(stderr, "Il tempo di delay deve essere un numero intero maggiore o uguale ad 0\n");
                    exit_check = 1;
                }
                break;
            case 'd':
                data->dname = strndup(optarg, PATH_MAX_LEN);
                int dlen = strlen(data->dname);
                if(dlen > PATH_MAX_LEN) {
                    fprintf(stderr,"Il nome directory non deve superare i 255 caratteri.\n");
                    exit_check = 1;
                }
                // Elimina il carattere '/' alla fine del path se è stato inserito
                if(data->dname[dlen-1] == '/') data->dname[dlen-1] = '\0';

                DIR *dir = opendir(data->dname);
                if (!dir) {
                    fprintf(stderr, "La directory inserita non e' valida: %s\n", data->dname);
                    exit_check = 1;
                }
                closedir(dir);
                dir_check = 1;
                break;
            case ':':
                if (optopt == 'n' || optopt == 'q' || optopt == 't' || optopt == 'd')
                    fprintf(stderr, "L'opzione -%c richiede un argomento\n", optopt);
                else fprintf(stderr, "Nessun argomento fornito al programma.\n");
                usage_help(argv[0]);
                exit_check = 1;
                break;
            case '?':
                fprintf(stderr, "Opzione -%c non riconosciuta\n", optopt);
                usage_help(argv[0]);
                exit_check = 1;
                break;
        }
    }

    // Esce se le opzioni in input non sono corrette
    if (exit_check == 1) {
        free_data(data);
        exit(EXIT_FAILURE);
    }


    // Esce se non esistono argomenti e -d non è settato
	if (data->num_file == 0 && dir_check == 0) {
		fprintf(stderr, "Nessun argomento fornito al programma.\n");
        usage_help(argv[0]);
        free_data(data);
		exit(EXIT_FAILURE);
	}

    // Inizializza la lista dei file passati come argomenti
    data->num_file = argc - optind;

    data->file_list = malloc(sizeof(char *)*data->num_file);
    if (data->file_list == NULL) {
        //perror("Errore allocazione data->file_list");
        fprintf(stderr, "Errore allocazione data->file_list");
        free_data(data);
        exit(EXIT_FAILURE);
    }

    // Rimpie la lista con gli argomenti inseriti
    int index = 0;
    FILE *fp;
    while (optind < argc) {
        if ((fp = fopen(argv[optind], "rb")) == NULL) {
            fprintf(stderr, "Argomento non valido: %s\n", argv[optind]);
            free_data(data);
            exit(EXIT_FAILURE);
        } else {
           data->file_list[index] = strndup(argv[optind], PATH_MAX_LEN);
        }
        fclose(fp);
        optind++;
        index++;
    }

    // Stampa la struttura dei dati di input (con verbose)
    if (verbose) {
        printf("MASTER DATA\n");
        printf("    nthread: %d\n", data->nthread);
        printf("       qlen: %d\n", data->qlen);
        printf("     tdelay: %ld\n", data->tdelay);
        printf("      dname: %s\n", data->dname);
        printf("   num_file: %d\n", data->num_file);
        printf("  file_list:\n");
        for(int i=0; i<data->num_file; i++) {
            printf("    -file %d: %s\n", i+1, data->file_list[i]);
        }
        printf("\n");
    }

    V_PRINT_MSG(FARM, "avvio");

	// Creazione del processo figlio
    V_PRINT_MSG(FARM, "crea processo collector");
    pid_t pid;
	pid = fork();
	if (pid < 0) {
		perror("errore nella creazione del secondo processo\n");
        free_data(data);
		exit(EXIT_FAILURE);
	}

	if(pid == 0) {
		// Figlio: Collector

        V_PRINT_MSG(FARM, "ha creato processo collector");
		collector_main();

	} else {
		// Padre: MasterWorker

        // Attende che collector abbia avviato la connessione
		sleepTime(200);

        V_PRINT_MSG(FARM,"avvia processo main di masterworker");
		masterWorker_main(data);

        // Attende la chiusura di Collector
		wait(NULL);

        V_PRINT_MSG(FARM,"chiusura");
	}

    // Libera la memoria della struttura dei dati di Masterworker
    free_data(data);

	return 0;
}

// Stampa un messaggio di aiuto sull'uso del programma
void usage_help(char* pname) {
    fprintf(stderr, "Usage: %s [-n nthread] [-q qlen] [-t tdelay] [-d dname] [-h] [file1 file2 ...]\n", pname);
}
