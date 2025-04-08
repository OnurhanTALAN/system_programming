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
#include <stdarg.h>

#define FIFO1 "/tmp/fifo1"
#define FIFO2 "/tmp/fifo2"
#define LOG_FILE "/tmp/daemon_log.txt"
#define PROCESS_TIMEOUT 30  // timeout in seconds

#define ABORT_EVERYTHING(msg)                \
    do{                                      \
        log_error("%s: %s", msg, strerror(errno)); \
        if(access(FIFO1, F_OK) == 0){        \
            unlink(FIFO1);                   \
        }                                    \
        if(access(FIFO2, F_OK) == 0){        \
            unlink(FIFO2);                   \
        }                                    \
        exit(EXIT_FAILURE);                  \
    }while(0)

volatile int exited_child_counter = 0;
volatile int num_children = 0;
volatile sig_atomic_t running = 1;

int string_to_int(const char *str);
int create_fifo(const char *path);
void first_child_process();
void second_child_process();
void sigchld_handler(int sig);
void sigusr1_handler(int sig);
void sighup_handler(int sig);
void sigterm_handler(int sig);
void log_message(const char *format, ...);
void log_error(const char *format, ...);
void cleanup();
void monitor_children();

pid_t pid1, pid2, pid_parent_write;
time_t child1_start_time, child2_start_time;
int log_fd;

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Invalid number of parameters.\nUsage %s <int1> <int2>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    if(pid < 0){
        perror("Failed to fork for daemon");
        return EXIT_FAILURE;
    }
    if(pid > 0){
        printf("Daemon started with PID %d\n", pid);
        exit(EXIT_SUCCESS);
    }

    if(setsid() < 0){
        perror("Failed to create new session");
        return EXIT_FAILURE;
    }

    pid = fork();
    if(pid < 0){
        perror("Second fork failed");
        return EXIT_FAILURE;
    }
    if(pid > 0){
        exit(EXIT_SUCCESS);
    }

    umask(0);
    chdir("/");

    for(int i = 0; i < sysconf(_SC_OPEN_MAX); i++){
        close(i);
    }

    log_fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(log_fd < 0){
        exit(EXIT_FAILURE);
    }
    dup2(log_fd, STDOUT_FILENO);
    dup2(log_fd, STDERR_FILENO);
    
    int dev_null = open("/dev/null", O_RDONLY);
    if(dev_null >= 0){
        dup2(dev_null, STDIN_FILENO);
        close(dev_null);
    }
    
/*    int result = 0;*/
/*    log_message("Daemon started. Result initialized to %d", result);*/

    int num1 = string_to_int(argv[1]);
    int num2 = string_to_int(argv[2]);

    log_message("Numbers received %d and %d", num1, num2);
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if(sigaction(SIGCHLD, &sa, NULL) == -1){
        ABORT_EVERYTHING("sigaction for SIGCHLD failed");
    }
    
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGUSR1, &sa, NULL) == -1){
        ABORT_EVERYTHING("sigaction for SIGUSR1 failed");
    }
    
    sa.sa_handler = sighup_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGHUP, &sa, NULL) == -1){
        ABORT_EVERYTHING("sigaction for SIGHUP failed");
    }
    
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGTERM, &sa, NULL) == -1){
        ABORT_EVERYTHING("sigaction for SIGTERM failed");
    }
    
    log_message("Signal handlers set up");
    
    if(create_fifo(FIFO1) == -1 || create_fifo(FIFO2) == -1){
        ABORT_EVERYTHING("Failed creating FIFOs");
    }
    log_message("FIFOs created successfully");
    
    pid_parent_write = fork();
    if(pid_parent_write < 0){
        ABORT_EVERYTHING("Fork failed for parent fifo write fork");
    }else if (pid_parent_write == 0){
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
        log_message("Parent writer: sent values %d and %d to FIFO1", num1, num2);
        exit(EXIT_SUCCESS);
    }
    num_children++;
    
    pid1 = fork();
    if(pid1 < 0){
        ABORT_EVERYTHING("Fork failed for child 1");
    }else if (pid1 == 0){
        first_child_process();
        exit(EXIT_FAILURE); // Should not reach here
    }
    child1_start_time = time(NULL);
    num_children++;
    
    pid2 = fork();
    if(pid2 < 0){
        ABORT_EVERYTHING("Fork failed for child 2");
    }else if (pid2 == 0){
        second_child_process();
        exit(EXIT_FAILURE); // Should not reach here
    }
    child2_start_time = time(NULL);
    num_children++;
    
    log_message("Parent: Created %d child processes", num_children);
    
    while(running && exited_child_counter < num_children){
        log_message("Parent: proceeding...");
        monitor_children();
        sleep(2);
    }
    
    log_message("Parent: All children have exited or program terminating");
    cleanup();
    return 0;
}

int string_to_int(const char *str){
    char *endptr;
    int num = (int)strtol(str, &endptr, 10);
    if(*endptr != '\0'){
        log_error("Error: '%s' is not a valid number", str);
        exit(EXIT_FAILURE);
    }
    return num;
}

int create_fifo(const char *path){
    if(access(path, F_OK) == 0){
        log_message("FIFO %s already exists, removing it...", path);
        if(unlink(path) == -1){
            log_error("Error: Could not remove existing FIFO %s", path);
            return -1;
        }
    }
    if(mkfifo(path, 0666) == -1){
        log_error("Error creating FIFO %s", path);
        return -1;
    }
    return 0;
}

void first_child_process(){
    log_message("Child 1 (PID: %d) started", getpid());
    log_message("Child 1: Sleeping for 10 seconds...");
    sleep(10);
    log_message("Child 1: Woke up after sleeping");
    
    int fifo1_fd = open(FIFO1, O_RDONLY | O_NONBLOCK);
    if (fifo1_fd < 0) {
        log_error("Child 1: Failed to open FIFO1");
        exit(EXIT_FAILURE);
    }
    
    int num1, num2;
    int retry_count = 0;
    const int max_retries = 5;
    ssize_t bytes_read1 = 0;
    ssize_t bytes_read2 = 0;
    
    while((bytes_read1 <= 0 || bytes_read2 <= 0) && retry_count < max_retries){
        if(bytes_read1 <= 0){
            bytes_read1 = read(fifo1_fd, &num1, sizeof(num1));
            if(bytes_read1 < 0 && errno != EAGAIN){
                close(fifo1_fd);
                log_error("Child 1: Error reading num1 from FIFO1");
                exit(EXIT_FAILURE);
            }
        }
        
        if(bytes_read1 > 0 && bytes_read2 <= 0){
            bytes_read2 = read(fifo1_fd, &num2, sizeof(num2));
            if(bytes_read2 < 0 && errno != EAGAIN){
                close(fifo1_fd);
                log_error("Child 1: Error reading num2 from FIFO1");
                exit(EXIT_FAILURE);
            }
        }
        
        if(bytes_read1 <= 0 || bytes_read2 <= 0){
            retry_count++;
            log_message("Child 1: Retry %d of %d for reading from FIFO1", retry_count, max_retries);
            sleep(1);
        }
    }
    
    close(fifo1_fd);
    
    if(bytes_read1 <= 0 || bytes_read2 <= 0){
        log_error("Child 1: Failed to read from FIFO1 after %d retries", max_retries);
        exit(EXIT_FAILURE);
    }
    
    log_message("Child 1: read integers %d and %d from FIFO1", num1, num2);
    
    int larger = (num1 > num2) ? num1 : num2;
    log_message("Child 1: %d is the larger number", larger);
    
    if(access(FIFO2, F_OK) != 0){
        log_error("Child 1: FIFO2 does not exist");
        exit(EXIT_FAILURE);
    }
    
    int fifo2_fd = open(FIFO2, O_WRONLY);
    if(fifo2_fd < 0){
        log_error("Child 1: Failed to open FIFO2 for writing");
        exit(EXIT_FAILURE);
    }
    
    log_message("Child 1: Writing larger value %d to FIFO2", larger);
    ssize_t bytes_written = write(fifo2_fd, &larger, sizeof(larger));
    if(bytes_written < 0){
        close(fifo2_fd);
        log_error("Child 1: Failed to write to FIFO2");
        exit(EXIT_FAILURE);
    }
        
    close(fifo2_fd);
    log_message("Child 1: Successfully wrote larger value %d to FIFO2", larger);
    log_message("Child 1: Completed its task and exiting");
    exit(EXIT_SUCCESS);
}

void second_child_process(){
    log_message("Child 2 (PID: %d) started", getpid());
    log_message("Child 2: Sleeping for 10 seconds...");
    sleep(10);
    log_message("Child 2: Woke up after sleeping");
       
    int fifo2_fd = open(FIFO2, O_RDONLY | O_NONBLOCK);         
    if(fifo2_fd < 0){
        log_error("Child 2: Failed to open FIFO2");
        exit(EXIT_FAILURE);
    }
    
    int larger;
    int retry_count = 0;
    const int max_retries = 5;
    ssize_t bytes_read = 0;
    
    log_message("Child 2: Reading from FIFO2");
    
    while(bytes_read <= 0 && retry_count < max_retries){
        bytes_read = read(fifo2_fd, &larger, sizeof(larger));
        if(bytes_read < 0) {
            if (errno != EAGAIN){
                close(fifo2_fd);
                log_error("Child 2: Error reading from FIFO2");
                exit(EXIT_FAILURE);
            }
        }else if (bytes_read == 0){
            log_message("Child 2: No data available yet on FIFO2");
        }
        
        if(bytes_read <= 0){
            retry_count++;
            log_message("Child 2: Retry %d of %d for reading from FIFO2", retry_count, max_retries);
            sleep(1);
        }
    }
    
    close(fifo2_fd);
    
    if(bytes_read <= 0){
        log_error("Child 2: Failed to read from FIFO2 after %d retries", max_retries);
        exit(EXIT_FAILURE);
    }
    
    log_message("Child 2: The larger number is %d", larger);
    log_message("Child 2: Completed its task and exiting");
    exit(EXIT_SUCCESS);
}

void sigchld_handler(int sig){
    (void)sig;
    pid_t pid;
    int status;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        exited_child_counter++;
        
        if(WIFEXITED(status)){
            log_message("Child process %d exited with status %d", pid, WEXITSTATUS(status));
        }else if (WIFSIGNALED(status)){
            log_message("Child process %d terminated by signal %d", pid, WTERMSIG(status));
        }
    }
}

void sigusr1_handler(int sig){
    (void)sig;
    log_message("Received SIGUSR1 signal");
}

void sighup_handler(int sig){
    (void)sig;
    log_message("Received SIGHUP signal - reconfiguring");
    
    close(log_fd);
    log_fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(log_fd >= 0){
        dup2(log_fd, STDOUT_FILENO);
        dup2(log_fd, STDERR_FILENO);
    }
    log_message("Log file reopened after SIGHUP");
}

void sigterm_handler(int sig){
    (void)sig;
    log_message("Received SIGTERM signal - shutting down gracefully");
    running = 0;
}

void cleanup(){
    log_message("Cleaning up resources");
    
    if(pid1 > 0){
        kill(pid1, SIGTERM);
    }
    if(pid2 > 0){
        kill(pid2, SIGTERM);
    }
    if(pid_parent_write > 0){
        kill(pid_parent_write, SIGTERM);
    }
    
    if(access(FIFO1, F_OK) == 0){
        unlink(FIFO1);
    }
    if(access(FIFO2, F_OK) == 0){
        unlink(FIFO2);
    }
    
    log_message("Cleanup complete. Daemon exiting.");
}

void monitor_children(){
    time_t current_time = time(NULL);
    
    if(pid1 > 0 && (current_time - child1_start_time) > PROCESS_TIMEOUT){
        log_message("Child 1 (PID: %d) timed out after %d seconds. Terminating.", pid1, PROCESS_TIMEOUT);
        kill(pid1, SIGTERM);
        pid1 = -1;
    }
    
    if(pid2 > 0 && (current_time - child2_start_time) > PROCESS_TIMEOUT){
        log_message("Child 2 (PID: %d) timed out after %d seconds. Terminating.", pid2, PROCESS_TIMEOUT);
        kill(pid2, SIGTERM);
        pid2 = -1;
    }
}

void log_message(const char *format, ...){
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    char timestamp[26];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    dprintf(STDOUT_FILENO, "[%s] ", timestamp);
    
    va_list args;
    va_start(args, format);
    vdprintf(STDOUT_FILENO, format, args);
    va_end(args);
    
    dprintf(STDOUT_FILENO, "\n");
}

void log_error(const char *format, ...){
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    char timestamp[26];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    dprintf(STDERR_FILENO, "[%s] ERROR: ", timestamp);
    
    va_list args;
    va_start(args, format);
    vdprintf(STDERR_FILENO, format, args);
    va_end(args);
    
    dprintf(STDERR_FILENO, "\n");
}
