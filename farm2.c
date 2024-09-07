#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "masterworker.h"
#include "collector.h"

#define NTHREAD_DEFAULT 4
#define QLEN_DEFAULT	8
#define TDELAY_DEFAULT	0

int main(int argc, char *argv[]){

	pid_t pid;

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
