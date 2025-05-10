#include "buffer.h"
#include <stdlib.h>
#include <stdio.h>

int init_buffer(Buffer* buffer, unsigned int cap){
    buffer->lines = (char**)malloc(cap * sizeof(char*));
    if(buffer->lines == NULL){
        printf("Error: Failed to allocate memory for buffer\n");
        return -1;
    }
    buffer->size = 0;
    buffer->capacity = cap;
    buffer->start = 0;
    buffer->end = 0;

    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->not_empty, NULL);
    pthread_cond_init(&buffer->not_full, NULL);

    return 0;
}

void destroy_buffer(Buffer* buffer){
    free(buffer->lines);
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->not_empty);
    pthread_cond_destroy(&buffer->not_full);
}

void insert_line(Buffer* buffer, char* line){
    pthread_mutex_lock(&buffer->mutex);

    while(buffer->size == buffer->capacity){
        printf("Buffer is full. Waiting for a slot to insert the line\n");
        pthread_cond_wait(&buffer->not_full, &buffer->mutex);
    }

    buffer->lines[buffer->end] = line;
    buffer->end = (buffer->end + 1) % buffer->capacity;
    buffer->size++;

    pthread_cond_signal(&buffer->not_empty);
    pthread_mutex_unlock(&buffer->mutex);
}

char* remove_line(Buffer* buffer){
    pthread_mutex_lock(&buffer->mutex);

    while(buffer->size == 0)
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
    

    char* line = buffer->lines[buffer->start];
    buffer->start = (buffer->start + 1) % buffer->capacity;
    buffer->size--;

    pthread_cond_signal(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);

    return line;
}
