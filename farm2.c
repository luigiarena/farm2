#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include "masterworker.h"
#include "collector.h"

#define MIN_NTHREAD			1
#define PATHNAME_LEN	  255
#define NTHREAD_DEFAULT 	4
#define QLEN_DEFAULT		8
#define TDELAY_DEFAULT		0

int main(int argc, char *argv[]){

	pid_t pid;
	int nthread;
	int qlen;

    // Analizza gli argomenti dati in input -n
    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
            case 'n':
                num_threads = atoi(optarg);
                if (num_threads <= 0) {
                    fprintf(stderr, "Il numero di thread deve essere maggiore di 0.\n");
                    exit(EXIT_FAILURE);
                }
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
