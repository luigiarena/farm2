/*
    PROGETTO FARM2
    Autore: Luigi Arena matricola 422353

    File: masterworker.h
    Descrizione: 
*/

/*
typedef struct worker_data {
    int worker_id;
    // questo void sarà castato per contenere un puntatore alla coda concorrente, qui non definita
    Coda *coda_link;
    // Serve?
    Worker_list *w_list_link;
} Worker_data;
*/

//static void *handler_signals(void *arg);
void explore_directory(const char *dname);
void masterWorker_main(char *file_list[], int list_num, int nthread, int qlen, char *dname);
