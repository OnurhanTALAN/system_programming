#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_CONNECTION_WINDOW 5
#define NUM_ENGINEER 3

int line_number = 0;

typedef struct{
	int id;
	int priority;
	bool is_handled;
	sem_t request_handled;
} Satellite;

typedef struct{
	Satellite** data;
	int size;
	int capacity;
} PriorityQueue;

bool isQueueEmpty();
Satellite* dequeue();
void enqueue(Satellite* sat);


int num_satellites;
PriorityQueue requestQueue;
int handledRequests = 0;
int abortedRequests = 0;
bool allRequestsHandled = false;

int availableEngineers = NUM_ENGINEER;
sem_t newRequest;
pthread_mutex_t engineerMutex = PTHREAD_MUTEX_INITIALIZER;

void* engineer(void* arg);
void* satellite(void* arg);

int main(){
	srand(time(NULL));
	num_satellites = (rand() % 10) + 5;
	requestQueue.data = malloc(sizeof(Satellite*) * num_satellites);
	requestQueue.size = 0;
	requestQueue.capacity = num_satellites;
	
	sem_init(&newRequest, 0, 0);

	pthread_t engineers[NUM_ENGINEER];
	for(int i = 0; i < NUM_ENGINEER; ++i){
		int* id = (int*)malloc(sizeof(int));
		*id = i;
		if(pthread_create(&engineers[i], NULL, engineer, id) != 0){
    		printf("Failed to create engineer thread");
    		free(id);
    		exit(EXIT_FAILURE);
		}
	}

	Satellite satellite_elements[num_satellites];
	pthread_t satellites[num_satellites];
	for(int i = 0; i < num_satellites; ++i){
		satellite_elements[i].id = i;
		satellite_elements[i].priority = (rand() % 5) + 1;
		satellite_elements[i].is_handled = false;
		sem_init(&satellite_elements[i].request_handled, 0, 0);
		
		if(pthread_create(&satellites[i], NULL, satellite, &satellite_elements[i]) != 0){
			printf("Failed to created satellite thread\n");
			exit(EXIT_FAILURE);
		}
	}
	

	for(int i = 0; i < num_satellites; ++i){
		pthread_join(satellites[i], NULL);
		sem_destroy(&satellite_elements[i].request_handled);
	}

	for(int i = 0; i < NUM_ENGINEER; ++i){
		pthread_join(engineers[i], NULL);
	}

	sem_destroy(&newRequest);
	pthread_mutex_destroy(&engineerMutex);
	free(requestQueue.data);

	printf("\n%d- All satellites processed.\nHandled: %d\nAborted: %d\n", ++line_number ,handledRequests, abortedRequests);

	return 0;
}

void* engineer(void* arg){
	int id = *(int*)arg;
	free(arg);
	while(true){
		sem_wait(&newRequest);
		pthread_mutex_lock(&engineerMutex);
		
		if(allRequestsHandled){
			pthread_mutex_unlock(&engineerMutex);
			break;
		}
		if(isQueueEmpty()){
			pthread_mutex_unlock(&engineerMutex);
			sleep(1);
			continue;
		}
		
		Satellite* satellite = dequeue();
		if(!satellite){
    		pthread_mutex_unlock(&engineerMutex);
    		continue;
		}
		availableEngineers--;
		printf("%d- [Engineer %d] Handling Satellite %d (Priority %d)\n", ++line_number, id, satellite->id, satellite->priority);
		
		satellite->is_handled = true;
		pthread_mutex_unlock(&engineerMutex);
		sem_post(&satellite->request_handled);
		
		int process_time = (rand() % 4) + 1;
		sleep(process_time);
		
		pthread_mutex_lock(&engineerMutex);
		
		availableEngineers++;
		printf("%d- [Engineer %d] Finished Satellite %d\n", ++line_number, id, satellite->id);
		
		pthread_mutex_unlock(&engineerMutex);
	}
	printf("[ENGINEER %d] exiting...\n", id);
	return NULL;
}

void* satellite(void* arg){
	Satellite* satellite = (Satellite*)arg;
	
	pthread_mutex_lock(&engineerMutex);
		printf("%d- [Satellite %d] Requesting (Priority %d)\n", ++line_number, satellite->id, satellite->priority);
		enqueue(satellite);
		sem_post(&newRequest);
	pthread_mutex_unlock(&engineerMutex);
	
	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += MAX_CONNECTION_WINDOW;
	
	int result = sem_timedwait(&satellite->request_handled, &timeout);
	
	pthread_mutex_lock(&engineerMutex);
	if(result == 0 || satellite->is_handled){
		handledRequests++;
	}else{
		printf("%d- [TIMEOUT] Satellite %d timeout %d seconds\n", line_number, satellite->id, MAX_CONNECTION_WINDOW);
		abortedRequests++;
	}
	
	if(handledRequests + abortedRequests >= num_satellites){
		allRequestsHandled = true;			
		for(int i = 0; i < NUM_ENGINEER; ++i){
			sem_post(&newRequest);
		}
	}
	pthread_mutex_unlock(&engineerMutex);
	return NULL;
}

bool isQueueEmpty(){ return requestQueue.size == 0; }

void enqueue(Satellite* sat){
    if(requestQueue.size >= requestQueue.capacity){
        printf("Queue is full!\n");
        return;
    }

    int i = requestQueue.size - 1;
    while(i >= 0 && requestQueue.data[i]->priority < sat->priority){
        requestQueue.data[i + 1] = requestQueue.data[i];
        i--;
    }
    requestQueue.data[i + 1] = sat;
    requestQueue.size++;
}

Satellite* dequeue(){
    if(isQueueEmpty()){ return NULL; }

    Satellite* sat = requestQueue.data[0];
    for(int i = 1; i < requestQueue.size; i++){
        requestQueue.data[i - 1] = requestQueue.data[i];
    }
    requestQueue.size--;
    return sat;
}


