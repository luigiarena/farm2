
typedef struct nodo {
    char *file_path;
    struct nodo *next;
} Nodo;

typedef struct coda {
    int size;
    Nodo *head;
    Nodo *tail;
    pthread_mutex_t lock;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} Coda;

Coda crea_coda();
void distruggi_coda();
int push_file(Coda *c, char *path);
char* pop_file(Coda *c);
int push();
int pop();

