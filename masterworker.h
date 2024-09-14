// header file per masterworker

void handler_signals(int sig_rec);
void explore_directory(const char *dname);
void MasterWorker(char *argv[], int argc, int opt, int nthread, int qlen, char *dname);
