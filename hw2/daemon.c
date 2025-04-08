#include "daemon.h"

volatile sig_atomic_t running = 1;

void daemonize(){
    pid_t pid;
    
    pid = fork();
    
    if(pid < 0){ exit(EXIT_FAILURE); }
    if(pid > 0){ exit(EXIT_SUCCESS); }
        
    if(setsid() < 0){ exit(EXIT_FAILURE); }
    
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, daemon_signal_handler);
    signal(SIGTERM, daemon_signal_handler);
    signal(SIGUSR1, daemon_signal_handler);
    
    pid = fork();
    
    if(pid < 0){ exit(EXIT_FAILURE); }
    if(pid > 0){ exit(EXIT_SUCCESS); }
    
    chdir("/");
    umask(0);
    
    int fd = open("/dev/null", O_RDWR);
    if(fd < 0){ exit(EXIT_FAILURE); }
	dup2(fd, STDIN_FILENO);
	close(fd);
    
    int log_fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(log_fd < 0){ exit(EXIT_FAILURE); }
    
    dup2(log_fd, STDOUT_FILENO);
    dup2(log_fd, STDERR_FILENO);
    
    for(int x = sysconf(_SC_OPEN_MAX); x >= 0; x--){
        if(x != STDOUT_FILENO && x != STDERR_FILENO && x != STDIN_FILENO){ close(x); }
    }
    daemon_log("Daemon process started");  
}

void daemon_log(const char *message){
    time_t now;
    time(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(stdout, "[%s] [PID: %d] %s\n", timestamp, getpid(), message);
    fflush(stdout);
}

void daemon_signal_handler(int sig) {
    char buffer[256];
    
    switch(sig){
        case SIGHUP:
            daemon_log("Received SIGHUP signal, reconfiguring...");
            break;
        case SIGTERM:
            daemon_log("Received SIGTERM signal, shutting down gracefully...");
            exit(EXIT_SUCCESS);
            break;
        case SIGUSR1:
            daemon_log("Received SIGUSR1 signal");
            break;
        default:
            snprintf(buffer, sizeof(buffer), "Received unsupported signal: %d", sig);
            daemon_log(buffer);
            break;
    }
}

void monitor_child_processes(pid_t *pids, int num_pids){
    time_t start_time = time(NULL);
    char buffer[256];
    
    daemon_log("Starting child process monitoring");
    
    for(int i = 0; i < num_pids; i++){
        snprintf(buffer, sizeof(buffer), "Monitoring child process: %d", pids[i]);
        daemon_log(buffer);
    }
    
    while(running) {
        time_t current_time = time(NULL);
        
        if(current_time - start_time > TIMEOUT_SECONDS) {
            daemon_log("Timeout monitoring child processes");
            for(int i = 0; i < num_pids; i++) {
                if(kill(pids[i], 0) == 0) {
                    snprintf(buffer, sizeof(buffer), "Killing unresponsive child process: %d", pids[i]);
                    daemon_log(buffer);
                    kill(pids[i], SIGTERM);
                }
            }
            break;
        }
        
        if((current_time - start_time) % 5 == 0) {
            daemon_log("Still monitoring child processes...");
        }
        
        sleep(1);
    }
    
    daemon_log("Child process monitoring terminated");
}

void monitor_ipc_activity() {
    char buffer[256];
    daemon_log("Starting IPC monitoring");
    
    // Set up non-blocking mode for FIFOs
    int fifo1_fd = open(FIFO1, O_RDONLY | O_NONBLOCK);
    if(fifo1_fd < 0) {
        snprintf(buffer, sizeof(buffer), "Failed to open FIFO1 for monitoring: %s", strerror(errno));
        daemon_log(buffer);
    } else {
        daemon_log("Successfully opened FIFO1 for monitoring");
    }
    
    int fifo2_fd = open(FIFO2, O_RDONLY | O_NONBLOCK);
    if(fifo2_fd < 0) {
        snprintf(buffer, sizeof(buffer), "Failed to open FIFO2 for monitoring: %s", strerror(errno));
        daemon_log(buffer);
    } else {
        daemon_log("Successfully opened FIFO2 for monitoring");
    }
    
    // Main monitoring loop
    int data;
    while(running) {
        ssize_t bytes_read;
        
        // Check FIFO1
        if(fifo1_fd >= 0) {
            bytes_read = read(fifo1_fd, &data, sizeof(data));
            if(bytes_read > 0) {
                snprintf(buffer, sizeof(buffer), "IPC activity detected on FIFO1: read %zd bytes", bytes_read);
                daemon_log(buffer);
            } else if(bytes_read < 0 && errno != EAGAIN) {
                snprintf(buffer, sizeof(buffer), "Error reading from FIFO1: %s", strerror(errno));
                daemon_log(buffer);
            }
        }
        
        // Check FIFO2
        if(fifo2_fd >= 0) {
            bytes_read = read(fifo2_fd, &data, sizeof(data));
            if(bytes_read > 0) {
                snprintf(buffer, sizeof(buffer), "IPC activity detected on FIFO2: read %zd bytes", bytes_read);
                daemon_log(buffer);
            } else if(bytes_read < 0 && errno != EAGAIN) {
                snprintf(buffer, sizeof(buffer), "Error reading from FIFO2: %s", strerror(errno));
                daemon_log(buffer);
            }
        }
        
        sleep(1);  // Reduced CPU usage
    }
    
    // Close FIFOs when done
    if(fifo1_fd >= 0) close(fifo1_fd);
    if(fifo2_fd >= 0) close(fifo2_fd);
    
    daemon_log("IPC monitoring terminated");
}


