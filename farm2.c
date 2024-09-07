#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include "masterworker.h"
#include "collector.h"

#define NTHREAD_MIN			1
#define NTHREAD_DEFAULT 	4
#define PATHNAME_LEN	  255
#define QLEN_DEFAULT		8
#define TDELAY_DEFAULT		0

// Funzione main per che lancia MasterWorker e fa partire il programma
int main(int argc, char *argv[]){

	// Dichiarazione e definizione delle variabili
	pid_t pid;
	int nthread = NTHREAD_DEFAULT;
	int qlen = QLEN_DEFAULT;
	int tdelay;
	char *dirname;

    // Analizza gli argomenti dati in input -n
	int opt;
    while ((opt = getopt(argc, argv, "n:q:d:t:")) != -1) {
        switch (opt) {
            case 'n':
                nthread = atoi(optarg);
                if (nthread < NTHREAD_MIN) {
                    fprintf(stderr, "Il numero di thread deve essere un numero intero maggiore o uguale ad 1 (default 4).\n");
                    exit(EXIT_FAILURE);
                }
                break;
			case 'q':
                qlen = atoi(optarg);
                break;
            case 't':
                tdelay = atoi(optarg);
                break;
            case 'd':
                dirname = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-n num_threads]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

	// Ignora SIGPIPE per tutti
	signal(SIGPIPE, SIG_IGN);

	// Creazione del processo figlio
	pid = fork();
	if (pid < 0) {
		perror("Errore nella creazione del secondo processo\n");
		exit(EXIT_FAILURE);
	}

	if(pid == 0) {
		// figlio: Collector
		Collector();
	} else {
		// padre: MasterWorker
		MasterWorker();
	}

	return 0;
}
