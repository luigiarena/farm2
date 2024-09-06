#include "utility.h"

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
