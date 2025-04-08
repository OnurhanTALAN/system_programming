CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = 

all: ipc_project

ipc_project: main.o daemon.o
	$(CC) $(CFLAGS) -o ipc_project main.o daemon.o $(LDFLAGS)

main.o: main.c daemon.h
	$(CC) $(CFLAGS) -c main.c

daemon.o: daemon.c daemon.h
	$(CC) $(CFLAGS) -c daemon.c

clean:
	rm -f *.o ipc_project /tmp/fifo1 /tmp/fifo2 /tmp/daemon_log.txt
