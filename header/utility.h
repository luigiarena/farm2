//#define _POSIX_C_SOURCE 1

#define FARM                        "FARM        "
#define MASTERWORKER                "MASTERWORKER"
#define COLLECTOR                   "COLLECTOR   "
#define WORKER                      "WORKER      "

#define V_PRINT_MSG(caller, msg)        if(verbose)fprintf(stderr,caller " -> " msg "\n");
#define V_PRINT_ARG(caller, msg,...)        if(verbose)fprintf(stderr,caller " -> " msg "\n",__VA_ARGS__);

void usage_help();