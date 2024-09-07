#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "masterworker.h"
#include "collector.h"

#define NTHREAD_MIN						1
#define NTHREAD_DEFAULT 				4
#define QLEN_DEFAULT					8
#define TDELAY_DEFAULT					0
#define DNAME_DEFAULT				 NULL
#define DNAME_PATHLEN				  255
#define SOCKETNAME			"./farm2.sck"
#define SOCKETNAME_LEN				   11

struct sockaddr_un sa;

// Funzione main per che lancia MasterWorker e fa partire il programma
int main(int argc, char *argv[]){

	// Dichiarazione e definizione delle variabili
	pid_t pid;
	int nthread = NTHREAD_DEFAULT;		// numero di threads
	int qlen = QLEN_DEFAULT;			// lunghezza della coda concorrente
	int tdelay = TDELAY_DEFAULT;							// tempo di ritardo nell'inserimento dei task
	char *dname = DNAME_DEFAULT;					// path della directory da visitare

    // Analizza i parametri dati in input -n
	int opt;
    while ((opt = getopt(argc, argv, "hn:q:d:t:")) != -1) {
        switch (opt) {
			case 'h':
    			printf("Options:\n");
    			printf("\t-n nthread\tnumero iniziale di thread worker\t(default 1)\n");
    			printf("\t-q qlen\t\tlunghezza della coda concorrente\t(default 8)\n");
    			printf("\t-t tdelay\tritardo inserimento task nella coda\t(default 0)\n");
    			printf("\t-d dname\tnaviga nella directory per cercare\n"
					   "\t\t\tfile da leggere in input\t\t(default .)\n");
    			exit(EXIT_SUCCESS);
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
                tdelay = atoi(optarg);
	            if (tdelay < 0) {
                    fprintf(stderr, "Il tempo di delay deve essere un numero intero maggiore o uguale ad 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'd':
                dname = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-n nthread] [-q qlen] [-t tdelay] [-d dname] [-h]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

	// Creo il file socket
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, SOCKETNAME, strlen(SOCKETNAME)+1);

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
