#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "utilities.h"
#include "fileUtils.h"
#include "directoryUtils.h"

typedef struct{
	char *command;
	void (*func)(char *args[], size_t argc);
} Command;

Command commands[] = {
	{"createDir", create_dir},
	{"createFile", create_file},
	{"listDir", list_dir},
	{"listFilesByExtension", list_dir_by_extension},
	{"readFile", read_file},
	{"appendToFile", append_to_file},
	{"deleteFile", delete_file},
	{"deleteDir", delete_dir},
	{"showlogs", show_logs}
};
#define NUM_COMMANDS (int)(sizeof(commands) / sizeof(commands[0]))

int main(int argc, char *argv[]){
	if(argc < 2){
		print_command_manual();
		return 0;
	}else{
		for(int i = 0; i < NUM_COMMANDS; ++i){
			if(strcmp(argv[1], commands[i].command) == 0){
				commands[i].func(argc > 2 ? &argv[2] : NULL, argc - 2);
				return 0;
			}
		}
	}
	printf("%s: command not found\n", argv[1]);
	return 0;
}



























