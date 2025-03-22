#include "fileUtils.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include "utilities.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define LOG_FILE "logs.txt"

void create_file(char *args[], size_t argc){
	if(argc == 0){
		no_filename_message();
		return;
	}
	
	for(size_t i = 0; i < argc; ++i){
		char *filename = args[i];
		if(file_exists(filename)){
			printf("Error: File %s already exists.\n", filename);
		}else{
			int file_descriptor = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
			if(file_descriptor == -1){
				printf("Error creating the file %s.\n", filename);
				continue;
			}
			char *timeStamp = get_timeStamp_string();
			ssize_t bytes_written = write(file_descriptor, timeStamp, strlen(timeStamp));
			if(bytes_written == -1){
				printf("Error writing to the file%s.\n", filename);
				close(file_descriptor);
				continue;
			}
			write_log("File %s created successfully.", filename);
			close(file_descriptor);
		}
	}
	return;
}

void read_file(char *args[], size_t argc){
	if(argc == 0){
		no_filename_message();
		return;
	}
	for(size_t i = 0; i < argc; ++i){
		char *filename = args[i];
		int file_descriptor = open(filename, O_RDONLY);
		if(file_descriptor == -1){
			printf("Error : File %s not found.\n", filename);
			continue;
		}
		char read_buffer[500];
		ssize_t bytes_read;
		bytes_read = read(file_descriptor, read_buffer, sizeof(read_buffer) - 1);
		if(bytes_read == -1){
			printf("Error : Could not read the file %s.\n", filename);
			close(file_descriptor);
			continue;
		}		
		read_buffer[bytes_read] = '\0';
		printf("%s\n", read_buffer);
		write_log("Content of the %s is displayed.", filename);
		close(file_descriptor);
	}
	return;	
}

void append_to_file(char *args[], size_t argc){
	if(!(argc >= 2)){
		printf("Error : Invalid number of arguments\n");
		return;
	}
	
	char *filename = args[0];
	
	if(!file_exists(filename)){
		printf("Error : File %s is not exists\n", filename);
		return;
	}
	int file_descriptor = open(filename, O_WRONLY | O_APPEND, 0644);
	if(file_descriptor == -1){
		printf("Error : Can not write to %s. File is locked or read-only\n", filename);
		return;
	}
	if(flock(file_descriptor, LOCK_EX) == -1){
		printf("Error : Failed to lock the file %s\n", filename);
		close(file_descriptor);
		return;
	}

	char *content = concatenate_strings(&args[1], argc - 1);
	if(content){
		if(write(file_descriptor, content, sizeof(char) * strlen(content)) == -1){
			printf("Error : Could not write to file %s\n", filename);
		}else{
			write_log("The new content is appended to file %s successfully.", filename);
		}
		free(content);
	} 
	flock(file_descriptor, LOCK_UN);
	close(file_descriptor);

}

void delete_file(char *args[], size_t argc){
	if(argc == 0){
		no_filename_message();
		return;
	}
	
	for(size_t i = 0; i < argc; ++i){
		pid_t pid = fork();
		if(pid < 0){
			printf("Error : Fork failed\n");
		}else if(pid == 0){
			char *filename = args[i];
			if(!file_exists(filename)){
				printf("Error : File %s not found\n", filename);
			}else{
				if(unlink(filename) == 0){
					write_log("File %s deleted successfully.", filename);
				}else{
					printf("File %s could not deleted\n", filename);
				}
			}
			exit(0);
		}else{
			wait(NULL);
		}
	}
}

void write_log(char *format, ...){
	char buffer[480];
	
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	
	int file_descriptor = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if(file_descriptor == -1){
		printf("Error : Could not open the logs file\n");
		return;
	}
	
	char *timeStamp = get_timeStamp_string();
	char log_entry[512];
	snprintf(log_entry, sizeof(log_entry), "%s %s\n", timeStamp, buffer);
	write(file_descriptor, log_entry, strlen(log_entry));
	close(file_descriptor);

}

void show_logs(char *args[], size_t argc){
	(void)args;
	(void)argc;
	if(!file_exists("logs.txt")){
		printf("Error : There is no log record\n");
	}else{
		int file_descriptor = open("logs.txt", O_RDONLY);
		if(file_descriptor == -1){
			printf("Error : Could not open the logs file\n");
			return;
		}
		char buffer[256];
		ssize_t bytes_read;
		
		while((bytes_read = read(file_descriptor, buffer, sizeof(buffer) - 1)) > 0){
			buffer[bytes_read] = '\0';
			write(STDOUT_FILENO, buffer, bytes_read);
			
		}
		if(bytes_read == -1){
			printf("Error : Could not read the logs file\n");
		}
		close(file_descriptor);
	}
}

int file_exists(char *path){
	return access(path, F_OK) == 0 ? 1 : 0;
}

void no_filename_message(){
	printf("Error : Filename is not provided\n");
}














