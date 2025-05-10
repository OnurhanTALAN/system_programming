#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "utils.h"
#include "buffer.h"

extern volatile sig_atomic_t should_exit;

int main(int argc, char *argv[]){
    setup_signal_handler();

    Params params = parse_args(argc, argv);
    print_params(params);

    int file_fd = open(params.log_file, O_RDONLY);
    if(file_fd == -1){
        printf("Error: Failed to open log file\n");
        exit(EXIT_FAILURE);
    }
    printf("Log file opened\n");

    Buffer buffer;
    if(init_buffer(&buffer, params.buffer_size) == -1){
        printf("Error: Failed to initialize buffer\n");
        cleanup(file_fd, &buffer, NULL, NULL, params.num_workers, NULL);
        exit(EXIT_FAILURE);
    }
    printf("Buffer initialized with capacity: %u\n", buffer.capacity);

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, params.num_workers + 1);
    printf("Barrier initialized\n");
    
    pthread_t* workers = NULL;
    WorkerParams* worker_params = NULL;
    if(create_workers(&workers, &worker_params, params.num_workers, &buffer, params.search_term, &barrier) == -1){
        printf("Error: Failed to create workers\n");
        cleanup(file_fd, &buffer, workers, worker_params, params.num_workers, &barrier);
        exit(EXIT_FAILURE);
    }

    char* line = NULL;
    while(!should_exit && (line = read_line(file_fd)) != NULL)
        insert_line(&buffer, line);

    
    insert_line(&buffer, NULL);

    pthread_barrier_wait(&barrier);
    for(unsigned int i = 0; i < params.num_workers; i++)
        pthread_join(workers[i], NULL);

    print_report(worker_params, params.num_workers);

    cleanup(file_fd, &buffer, workers, worker_params, params.num_workers, &barrier);
    return 0;
}
