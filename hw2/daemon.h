/* daemon.h - Header file for daemon functionality */
#ifndef DAEMON_H
#define DAEMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define LOG_FILE "/tmp/daemon.txt"
#define TIMEOUT_SECONDS 60
#define FIFO1 "/tmp/fifo1"
#define FIFO2 "/tmp/fifo2"


void daemonize();

void daemon_log(const char *message);

void daemon_signal_handler(int sig);

void monitor_child_processes(pid_t *pids, int num_pids);

void monitor_ipc_activity();

#endif /* DAEMON_H */
