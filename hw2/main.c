#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include "daemon.h"

#define FIFO1 "/tmp/fifo1"
#define FIFO2 "/tmp/fifo2"

#define ABORT_EVERYTHING(msg)            \
    do{                                  \
        perror(msg);                      \
        if(access(FIFO1, F_OK) == 0){   \
            unlink(FIFO1);                \
        }                                 \
        if(access(FIFO2, F_OK) == 0){   \
            unlink(FIFO2);                \
        }                                 \
        exit(EXIT_FAILURE);               \
    }while(0)

volatile int exited_child_counter = 0;
volatile int num_children = 0;

int string_to_int(const char *str);
int create_fifo(const char *path);

void first_child_process();
void second_child_process();

void sigchld_handler(int sig);

pid_t pid1, pid2;

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Invalid number of parameters.\nUsage %s <int1> <int2>\n", argv[0]);
        return 1;
    }

    int num1 = string_to_int(argv[1]);
    int num2 = string_to_int(argv[2]);
    int result = 0;
    printf("Numbers received %d and %d\n", num1, num2);
    
    if(create_fifo(FIFO1) == -1 || create_fifo(FIFO2) == -1){
        ABORT_EVERYTHING("Failed creating FIFOs");
    }
    printf("FIFOs are created successfully\n");

    struct sigaction signal_action;
    memset(&signal_action, 0, sizeof(signal_action));
    signal_action.sa_handler = sigchld_handler;
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if(sigaction(SIGCHLD, &signal_action, NULL) == -1){
        ABORT_EVERYTHING("sigaction failed");
    }
    printf("Signal handler set up\n");
    
    pid1 = fork();
    if(pid1 < 0){
        ABORT_EVERYTHING("Fork failed for child 1");
    }else if(pid1 == 0){
        first_child_process();
        exit(EXIT_FAILURE);
    }
    num_children++;
    
    pid2 = fork();
    if(pid2 < 0) {
        ABORT_EVERYTHING("Fork failed for child 2");
    } else if(pid2 == 0) {
        second_child_process();
        exit(EXIT_FAILURE);
    }
    num_children++;

    pid_t daemon_pid = fork();
    if(daemon_pid < 0){
        ABORT_EVERYTHING("Fork failed for daemon");
    }else if(daemon_pid == 0){
        printf("Starting daemon process\n");
        daemonize();
        
        exit(EXIT_SUCCESS);
    }
    num_children++;
    
    pid_t pid3 = fork();
    if(pid3 < 0){
    	ABORT_EVERYTHING("Fork failed for child 3");
    }else if(pid3 == 0){
	    int fifo1_fd = open(FIFO1, O_WRONLY);
	    if(fifo1_fd < 0){
	        ABORT_EVERYTHING("Parent: Failed to open FIFO1");
	    }
    
	    if(write(fifo1_fd, &num1, sizeof(num1)) == -1 ||
	       write(fifo1_fd, &num2, sizeof(num2)) == -1){
	        close(fifo1_fd);
	        ABORT_EVERYTHING("Parent: Failed to write to FIFO1");
	    }
	    close(fifo1_fd);
	    printf("Parent: sent values %d and %d to FIFO1\n", num1, num2);
	    exit(EXIT_SUCCESS);
    }
    num_children++;
    
    while(exited_child_counter < num_children * 2){
        printf("Parent: proceeding...\n");
        sleep(2);
    }

    printf("Parent: All children have exited, cleaning up\n");
    unlink(FIFO1);
    unlink(FIFO2);
    return 0;
}

int string_to_int(const char *str){
    char *endptr;
    int num = (int)strtol(str, &endptr, 10);
    
    if(*endptr != '\0'){
        printf("Error : '%s' is not a number.\n", str);
        exit(EXIT_FAILURE);
    }
    return num;
}

int create_fifo(const char *path){
    if(access(path, F_OK) == 0){
        printf("FIFO %s already exists, removing it...\n", path);
        if(unlink(path) == -1){
            perror("Error: Could not remove existing FIFO");
            return -1;
        }
    }
    if(mkfifo(path, 0666) == -1){
        perror("Error creating FIFO");
        return -1;
    }
    return 0;
}

void first_child_process(){
    printf("Child 1 (PID : %d) started\n", getpid());
    printf("Child 1: Sleeping for 10 seconds...\n");
    sleep(10);
    printf("Child 1: Woke up after sleeping\n");
    
    int fifo1_fd = open(FIFO1, O_RDONLY);
    if(fifo1_fd < 0){ 
        ABORT_EVERYTHING("Child 1: open FIFO1"); 
    }
    
    int num1, num2;
    ssize_t bytes_read1 = read(fifo1_fd, &num1, sizeof(num1));
    if(bytes_read1 < 0){
        close(fifo1_fd);
        ABORT_EVERYTHING("Child 1: read num1 from FIFO1");
    }

    ssize_t bytes_read2 = read(fifo1_fd, &num2, sizeof(num2));
    if(bytes_read2 < 0){
        close(fifo1_fd);    
        ABORT_EVERYTHING("Child 1: read num2 from FIFO1");
    }
    
    close(fifo1_fd);
    printf("Child 1: read integers %d and %d from FIFO1\n", num1, num2);
    
    int larger = (num1 > num2) ? num1 : num2;
    printf("Child 1: %d is the larger number\n", larger);
    
    if(access(FIFO2, F_OK) != 0){
        ABORT_EVERYTHING("Child 1: FIFO2 does not exist");
    }
    
    int fifo2_fd = open(FIFO2, O_WRONLY);
    if(fifo2_fd < 0){ 
        ABORT_EVERYTHING("Child 1: open FIFO2");
    }
    
    printf("Child 1: Writing larger value %d to FIFO2\n", larger);
    ssize_t bytes_written = write(fifo2_fd, &larger, sizeof(larger));
    if(bytes_written < 0){
        close(fifo2_fd);        
        ABORT_EVERYTHING("Child 1: write to FIFO2");
    }
        
    close(fifo2_fd);
    printf("Child 1: Successfully wrote larger value %d to FIFO2\n", larger);
    printf("Child 1: Completed its task and exiting\n");
    exit(EXIT_SUCCESS);
}

void second_child_process(){
    printf("Child 2 (PID : %d) started\n", getpid());
    printf("Child 2: Sleeping for 10 seconds...\n");
    sleep(10);
    printf("Child 2: Woke up after sleeping\n");
       
    int fifo2_fd = open(FIFO2, O_RDONLY); 
            
    if(fifo2_fd < 0){
        ABORT_EVERYTHING("Child 2: open FIFO2");
    }
    
    int larger;
    printf("Child 2: Reading from FIFO2\n");
    ssize_t bytes_read = read(fifo2_fd, &larger, sizeof(larger));
    
    if(bytes_read < 0){
        close(fifo2_fd);
        ABORT_EVERYTHING("Child 2: read from FIFO2");
    }else if(bytes_read == 0){
        close(fifo2_fd);
        printf("Child 2: End of file reached on FIFO2 (writer closed)\n");
        ABORT_EVERYTHING("Child 2: No data read from FIFO2");
    }
    
    close(fifo2_fd);
    printf("Child 2: The larger number is %d\n", larger);
    printf("Child 2: Completed its task and exiting\n");
    exit(EXIT_SUCCESS);
}

void sigchld_handler(int sig){
    (void)sig;
    pid_t pid;
    int status;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
    	exited_child_counter += 2;
		if(WIFEXITED(status)){
        	printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
        }else if(WIFSIGNALED(status)){
        	printf("Child process %d terminated by signal %d\n", pid, WTERMSIG(status));
        }
    }
}
