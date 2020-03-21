#ifndef GETOPT_HELPER_H
#define GETOPT_HELPER_H

#include <getopt.h>

// ---------------------------------------------------------------------------
typedef struct {
    const char* name;
    int has_arg;
    int* flag;
    int val;
    const char* description;
    const char* arg_name;
} getopt_arg_t;


// ---------------------------------------------------------------------------
typedef struct {
    int single;
    int presenter;
    char* pdf;
} Options;


// ---------------------------------------------------------------------------
struct option* getopt_get_long_options(getopt_arg_t* opt);
void show_usage(char *binary, getopt_arg_t* cli_options);
int parse_cli_options(Options* options, getopt_arg_t* cli_options, int argc, char* argv[]);

#endif 
