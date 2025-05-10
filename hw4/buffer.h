#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>

typedef struct{
    char** lines;
    unsigned int size;
    unsigned int capacity;

    unsigned int start;
    unsigned int end;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;

} Buffer;

int init_buffer(Buffer* buffer, unsigned int cap);
void destroy_buffer(Buffer* buffer);

void insert_line(Buffer* buffer, char* line);
char* remove_line(Buffer* buffer);

#endif