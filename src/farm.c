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
//#define SOCKET_PATH			"./farm2.sck"
//#define SOCKET_PATH_LEN				   11
//#define BUF_MAX_SIZE                  255

#define len_verbose 20

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
	int tdelay = TDELAY_DEFAULT;		// tempo di ritardo nell'inserimento dei task
	char *dname = NULL;		            // path della directory da visitare

    // Analizza i parametri dati in input -n
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
                tdelay = atoi(optarg);
	            if (tdelay < 0) {
                    fprintf(stderr, "Il tempo di delay deve essere un numero intero maggiore o uguale ad 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'd':
                dname = optarg;
                //if (dname != NULL) list_index = add_dir(dname, file_list, list_index);
                break;
            default:
            usage_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    V_PRINT_MSG(FARM, "apertura");

    // Se non esistono argomenti e -d non è settato chiudo
    int num_file = 0;
	if (optind >= argc && dname == NULL) {
		fprintf(stderr, "Nessun argomento fornito al programma.\n");
        usage_help(argv[0]);
		exit(EXIT_FAILURE);
	} else {
        // Altrimenti inizializzo un array dei file inseriti come argomenti
        num_file = argc-optind;
        if (dname != NULL) num_file++;
    }

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
    if (dname != NULL) strncpy(file_list[index], dname, PATH_MAX_LEN);
    // TEST
    printf("Numero di argomenti: %d\n", num_file);

    for(int i=0; i<num_file; i++) printf("file n.%d: %s\n", i, file_list[i]);

	// Creo il file socket SERVE??
    //sa.sun_family = AF_UNIX;
    //strncpy(sa.sun_path, SOCKET_PATH, strlen(SOCKET_PATH)+1);

	// Ignora SIGPIPE per tutti
	//signal(SIGPIPE, SIG_IGN);

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
		sleep(1); // Attendo che collector abbia avviato la connessione

        V_PRINT_MSG(FARM,"avvio processo main di masterworker");
		masterWorker_main(file_list, num_file, nthread, qlen, dname);

        V_PRINT_MSG(MASTERWORKER,"attendo chiusura di collector");
		wait(NULL); // Attendo la chiusura di Collector

        V_PRINT_MSG(FARM,"chiusura");
	}

	return 0;
}

// Stampa un messaggio di aiuto sull'uso del programma
void usage_help(char* pname) {
    fprintf(stderr, "Usage: %s [-n nthread] [-q qlen] [-t tdelay] [-d dname] [-h] [file1 file2 ...]\n", pname);
}
/*
// Funzione che esplora la directory, saltando file ., .. e nascosti
int add_dir(const char *dname, char *ar[], int index) {
    struct dirent *entry;
    struct stat file_stat;

    DIR *dir = opendir(dname);
    if (!dir) {
        perror("Errore nell'aprire la directory dei file di input");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[1024];

        // Salta "." e ".." e i file nascosti
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Crea il path completo
        snprintf(full_path, sizeof(full_path), "%s/%s", dname, entry->d_name);

        // Ottieni informazioni sul file
        if (stat(full_path, &file_stat) == -1) {
            perror("Errore nell'ottenere informazioni sul file");
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            // Se è una directory la esplora ricorsivamente
            //--printf("Directory: %s\n", full_path);
            add_dir(full_path, ar, index);
        } else if (S_ISREG(file_stat.st_mode)) {
            // Se è un file regolare lo aggiungo alla coda concorrente
            // printf("File regolare: %s\n", full_path);
			strncpy(ar[index], full_path, BUF_MAX_SIZE);
			index++;
        }
    }

    closedir(dir);
	return index;
}
*/