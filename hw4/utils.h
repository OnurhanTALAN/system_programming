#ifndef UTILS_H
#define UTILS_H

#define _POSIX_C_SOURCE 200809L
#include "buffer.h"

#define NUM_PARAMS 5
#define READ_LINE_BUFFER_SIZE 1024
#define MAX_MATCHING_LINES 100

typedef struct{
    unsigned int buffer_size;
    unsigned int num_workers;
    const char* log_file;
    const char* search_term;
} Params;

typedef struct{
    unsigned int* worker_id;
    Buffer* buffer;
    const char* search_term;
    pthread_barrier_t* barrier;
    unsigned int num_matches;
    char** matching_lines;
    unsigned int matching_lines_index;
} WorkerParams;

void sigint_handler(int signum);
void setup_signal_handler(void);


Params parse_args(int argc, char *argv[]);
void print_params(Params params);
unsigned int string_to_int(const char *str);

char* read_line(int file_fd);

int create_workers(pthread_t** workers, WorkerParams** worker_params, unsigned int num_workers, Buffer* buffer, const char* search_term, pthread_barrier_t* barrier);
void* worker_thread(void* arg);

void cleanup(int file_fd, Buffer* buffer, pthread_t* workers, WorkerParams* worker_params, unsigned int num_workers, pthread_barrier_t* barrier);
void print_report(WorkerParams* worker_params, unsigned int num_workers);

#endif

