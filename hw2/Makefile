CFLAGS = -Wall -Wextra -g

all: main

main: main.c
	gcc $(CFLAGS) -o main main.c

clean:
	rm -f main
	rm -f /tmp/fifo1
	rm -f /tmp/fifo2
	rm -f /tmp/daemon_log.txt
