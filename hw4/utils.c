#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

volatile sig_atomic_t should_exit = 0;

void sigint_handler(int signum){
    (void)signum;
    should_exit = 1;
    printf("\nReceived SIGINT. Cleaning up...\n");
}

void setup_signal_handler(void){
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGINT, &sa, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

Params parse_args(int argc, char *argv[]){
    if(argc != NUM_PARAMS){
        printf("Usage: %s <buffer_size> <num_workers> <log_file> <search_term>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    unsigned int buffer_size = string_to_int(argv[1]);
    unsigned int num_workers = string_to_int(argv[2]);
    const char* log_file = argv[3];
    if(access(log_file, F_OK) == -1){
        printf("Error: log file '%s' does not exist\n", log_file);
        exit(EXIT_FAILURE);
    }
    const char* search_term = argv[4];
    Params params = {buffer_size, num_workers, log_file, search_term};
    return params;
}

void print_params(Params params){
    printf("Buffer size: %u\n", params.buffer_size);
    printf("Number of workers: %u\n", params.num_workers);
    printf("Log file: %s\n", params.log_file);
    printf("Search term: %s\n", params.search_term);
    printf("==========================================\n");
}

unsigned int string_to_int(const char *str){
    char *endptr;
    long num = strtol(str, &endptr, 10);
    if(*endptr != '\0'){
        printf("Error: '%s' is not a valid number", str);
        exit(EXIT_FAILURE);
    }
    if(num <= 0){
        printf("Error: '%s' is not a positive number\n", str);
        exit(EXIT_FAILURE);
    }
    return (unsigned int)num;
}

char* read_line(int file_fd){
    char* buffer = (char*)malloc(READ_LINE_BUFFER_SIZE * sizeof(char));
    if(buffer == NULL){
        printf("Error: Failed to allocate memory for buffer\n");
        return NULL;
    }
    
    ssize_t total_read = 0;
    ssize_t bytes_read = 0;

    while(total_read < READ_LINE_BUFFER_SIZE - 1){
        char ch;
        bytes_read = read(file_fd, &ch, sizeof(ch));
        if(bytes_read == -1){
            printf("Error: Failed to read from file\n");
            free(buffer);
            return NULL;
        }
        if(bytes_read == 0){ break;}
        if(ch == '\n'){ break;}
        buffer[total_read++] = ch;
    }

    if(total_read == 0){
        free(buffer);
        return NULL;
    }

    buffer[total_read] = '\0';
    return buffer;
}

int create_workers(pthread_t** workers, WorkerParams** worker_params, unsigned int num_workers, Buffer* buffer, const char* search_term, pthread_barrier_t* barrier){
    *workers = (pthread_t*)malloc(num_workers * sizeof(pthread_t));
    *worker_params = (WorkerParams*)malloc(num_workers * sizeof(WorkerParams));
    if(*workers == NULL || *worker_params == NULL){
        printf("Error: Failed to allocate memory for workers or worker params\n");
        return -1;
    }

    for(unsigned int i = 0; i < num_workers; i++){
        (*worker_params)[i].buffer = buffer;
        (*worker_params)[i].search_term = search_term;
        (*worker_params)[i].barrier = barrier;
        (*worker_params)[i].num_matches = 0;
        (*worker_params)[i].matching_lines = (char**)malloc(MAX_MATCHING_LINES * sizeof(char*));
        if((*worker_params)[i].matching_lines == NULL){
            printf("Error: Failed to allocate memory for matching lines\n");
            return -1;
        }
        (*worker_params)[i].matching_lines_index = 0;
        unsigned int* worker_id = (unsigned int*)malloc(sizeof(unsigned int));
        if(worker_id == NULL){
            printf("Error: Failed to allocate memory for worker id\n");
            return -1;
        }
        *worker_id = i;
        (*worker_params)[i].worker_id = worker_id;

        if(pthread_create(&(*workers)[i], NULL, worker_thread, &(*worker_params)[i]) != 0){
            printf("Error: Failed to create worker thread %u\n", i);
            return -1;
        }
    }
    return 0;
}

void* worker_thread(void* arg){
    WorkerParams* params = (WorkerParams*)arg;
    printf("Worker %u started\n", *params->worker_id);

    while(!should_exit){
        char* line = remove_line(params->buffer);
        if(line == NULL){
            printf("Worker %u: Got NULL line. Stop working\n", *params->worker_id);
            insert_line(params->buffer, NULL);
            break;
        }

        const char* pos = line;
        int found = 0;
        while((pos = strstr(pos, params->search_term)) != NULL){
            params->num_matches++;
            if(!found){
                params->matching_lines[params->matching_lines_index++] = strdup(line);
                found = 1;
            }
            pos++;
        }
        free(line);
    }
    if(should_exit){
        printf("[WORKER %u] Received SIGINT. Exiting...\n", *params->worker_id);
        return NULL;
    }

    printf("[WORKER %u] Finished. Waiting for other workers...\n", *params->worker_id);
    pthread_barrier_wait(params->barrier);

    printf("[WORKER %u] Total matches found: %u\n", *params->worker_id, params->num_matches);
    
    return NULL;
}

void cleanup(int file_fd, Buffer* buffer, pthread_t* workers, WorkerParams* worker_params, unsigned int num_workers, pthread_barrier_t* barrier){
    printf("\n===CLEANING UP===\n");
    if(file_fd != -1){
        close(file_fd);
        printf("File closed\n");
    }

    if(buffer != NULL){
        destroy_buffer(buffer);
        printf("Buffer destroyed\n");
    }

    if(workers != NULL){
        free(workers);
        printf("Workers freed\n");
    }
    
    if(worker_params != NULL){
        for(unsigned int i = 0; i < num_workers; i++){
            if(worker_params[i].worker_id != NULL)
                free(worker_params[i].worker_id);
            if(worker_params[i].matching_lines != NULL){
                for(unsigned int j = 0; j < worker_params[i].num_matches; j++){
                    free(worker_params[i].matching_lines[j]);
                }
                free(worker_params[i].matching_lines);
            }
        }
        free(worker_params);
        printf("Worker params freed\n");
    }

    if(barrier != NULL){
        pthread_barrier_destroy(barrier);
        printf("Barrier destroyed\n");
    }

    printf("===CLEANED UP===\n");
}

void print_report(WorkerParams* worker_params, unsigned int num_workers){
    printf("\n========REPORT========\n");
    for(unsigned int i = 0; i < num_workers; i++){
        printf("\n----[Worker %u] %u matches----\n", i, worker_params[i].num_matches);
        for(unsigned int j = 0; j < worker_params[i].matching_lines_index; j++){
            printf("%u- %s\n", j+1, worker_params[i].matching_lines[j]);
        }
    }
    printf("========END OF REPORT========\n");
}
