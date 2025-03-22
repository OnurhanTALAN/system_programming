all: fileManager

fileManager: fileManager.o utilities.o fileUtils.o directoryUtils.o
	gcc -Wall -Wextra -std=c11 fileManager.o utilities.o fileUtils.o directoryUtils.o -o fileManager 

fileManager.o: fileManager.c
	gcc -Wall -Wextra -std=c11 -c fileManager.c -o fileManager.o

utilities.o: utilities.c
	gcc -Wall -Wextra -std=c11 -c utilities.c -o utilities.o

fileUtils.o: fileUtils.c
	gcc -Wall -Wextra -std=c11 -c fileUtils.c -o fileUtils.o
	
directoryUtils.o: directoryUtils.c
	gcc -Wall -Wextra -std=c11 -c directoryUtils.c -o directoryUtils.o

clean:
	rm -f fileManager.o utilities.o fileUtils.o directoryUtils.o fileManager
	
rebuild: clean all

run: rebuild fileManager
	./fileManager $(ARGS)
