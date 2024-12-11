// header file per masterworker

void handler_signals(int sig_rec);
void explore_directory(const char *dname);
void masterWorker_main(char *file_list[], int list_index, int nthread, int qlen, char *dname);
