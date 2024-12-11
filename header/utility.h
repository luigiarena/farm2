//#define _POSIX_C_SOURCE 1

#define VERBOSE_PRINT_OLD(msg)          if(verbose)fprintf(stderr,msg);
#define VERBOSE_PRINT(msg, ...)     if(verbose)fprintf(stderr,msg, ##__VA_ARGS__);

void usage_help();