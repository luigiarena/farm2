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
    master_data_t * data = read_opt(argc, argv);

    // Stampa la struttura dei dati di input (con verbose)
    if (verbose) {
        printf("MASTER DATA\n");
        printf("  nthread: %d\n", data->nthread);
        printf("     qlen: %d\n", data->qlen);
        printf("   tdelay: %ld\n", data->tdelay);
        printf("    dname: %s\n", data->dname);
        printf(" num_file: %d\n", data->num_file);
        printf("file_list:\n");
        for(int i=0; i<data->num_file; i++) {
            printf("  file %d %s\n", i+1, data->file_list[i]);
        }
        printf("\n");
    }

    // Se non esistono argomenti e -d non è settato chiude
	if (data->num_file < 1 && data->dname == NULL) {
		fprintf(stderr, "nessun argomento fornito al programma.\n");
        usage_help(argv[0]);
		exit(EXIT_FAILURE);
	}

    V_PRINT_MSG(FARM, "avvio");

	// Creazione del processo figlio
    V_PRINT_MSG(FARM, "crea processo collector");
    pid_t pid;
	pid = fork();
	if (pid < 0) {
		perror("errore nella creazione del secondo processo\n");
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

	return 0;
}

// Stampa un messaggio di aiuto sull'uso del programma
void usage_help(char* pname) {
    fprintf(stderr, "Usage: %s [-n nthread] [-q qlen] [-t tdelay] [-d dname] [-h] [file1 file2 ...]\n", pname);
}

// Riceve i dati di input, li analizza e costruisce la struttura necessaria al funzionamento di masterworker
master_data_t *read_opt(int argc, char *argv[]) {
    master_data_t *data = malloc(sizeof(master_data_t));
    ec_val(data, NULL, "Errore malloc master_data");

    // Inizializza i valori della struttura di Masterworker
    data->nthread = NTHREAD_DEFAULT;
    data->qlen = QLEN_DEFAULT;
    data->tdelay = TDELAY_DEFAULT;
    data->num_file = 0;

    // Strutture di appoggio per creare la lista dei file/argomento
    int file_temp[argc];
    int index_temp = 0;

    // Ciclo di analisi dei dati di input (argv[])
    int index = 1;
    char *opt;
    while (index < argc) {
        opt = argv[index];
        if (opt[0] == '-') {
            if (opt[1] != '\0' && opt[2] == '\0') {
                switch (opt[1]) {
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
                        index++;
                        if (index >= argc) {
                            fprintf(stderr, "Valore di -n non valido\n");
                            exit(EXIT_FAILURE);
                        }
                        opt = argv[index];
                        data->nthread = atoi(opt);
                        if (data->nthread < NTHREAD_MIN) {
                            fprintf(stderr, "Il numero di thread deve essere un numero intero maggiore o uguale ad %d\n", NTHREAD_MIN);
                            exit(EXIT_FAILURE);
                        }
                        break;
                    case 'q':
                        index++;
                        if (index >= argc) {
                            fprintf(stderr, "Valore di -q non valido\n");
                            exit(EXIT_FAILURE);
                        }
                        opt = argv[index];
                        data->qlen = atoi(opt);
                        if (data->qlen < 1) {
                            fprintf(stderr, "La lunghezza della coda deve essere un numero intero maggiore o uguale ad 1\n");
                            exit(EXIT_FAILURE);
                        }
                        break;
                    case 't':
                        index++;
                        if (index >= argc) {
                            fprintf(stderr, "Valore di -t non valido\n");
                            exit(EXIT_FAILURE);
                        }
                        opt = argv[index];
                        data->tdelay = atol(opt);
                        if (data->tdelay < 0) {
                            fprintf(stderr, "Il tempo di delay deve essere un numero intero maggiore o uguale ad 0\n");
                            exit(EXIT_FAILURE);
                        }
                        break;
                    case 'd':
                        index++;
                        if (index >= argc) {
                            fprintf(stderr, "Valore di -d non valido\n");
                            exit(EXIT_FAILURE);
                        }
                        opt = argv[index];
                        if(strlen(opt) > PATH_MAX_LEN) {
                            fprintf(stderr,"Nome directory troppo lungo (MAX 255 caratteri).\n");
                            exit(EXIT_FAILURE);
                        }
                        // Elimina il carattere '/' alla fine del path se è stato inserito
                        if(opt[strlen(opt)-1] == '/') opt[strlen(opt)-1] = '\0';

                        data->dname = strndup(opt, sizeof(opt));
                        DIR *dir = opendir(data->dname);
                        if (!dir) {
                            fprintf(stderr, "La directory inserita non e' valida: %s\n", opt);
                            exit(EXIT_FAILURE);
                        }
                        closedir(dir);
                        break;
                    default:
                        fprintf(stderr, "Opzione non riconosciuta: %s\n", opt);
                        usage_help(argv[0]);
                        exit(EXIT_FAILURE);
                        break;
                }
            } else {
                fprintf(stderr, "Opzione non riconosciuta: %s\n", opt);
            }
        } else {
            if (fopen(opt, "rb") == NULL) {
                fprintf(stderr, "Argomento non valido: %s\n", opt);
                exit(EXIT_FAILURE);
            } else {
                file_temp[index_temp] = index;
                index_temp++;
                data->num_file++;
            }
        }
        index++;
    }

    // Inizializza la lista dei file passati come argomenti
    data->file_list = malloc(sizeof(char *)*data->num_file);
    index = 0;
    // Rimpie la lista con gli argomenti inseriti
    while (index < data->num_file) {
        data->file_list[index] = malloc(PATH_MAX_LEN);
        strncpy(data->file_list[index], argv[file_temp[index]], PATH_MAX_LEN);
        index++;
    }

    return data;
}