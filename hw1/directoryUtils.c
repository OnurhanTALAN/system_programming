#include "directoryUtils.h"
#include "fileUtils.h"
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

void create_dir(char *args[], size_t argc){
	if(argc == 0){
		no_directory_message();
		return;	
	}
	for(size_t i = 0; i < argc; ++i){
		char *dirname = args[i];
		if(directory_exists(dirname)){
			printf("Error : Directory %s already exists\n", dirname);
			continue;
		}
		if(mkdir(args[i], 0755) == 0){
			write_log("Directory %s created successfully.", dirname);
		}else{
			printf("Error creating directory %s\n", dirname);
		}
	}
}

void delete_dir(char * args[], size_t argc){
	if(argc == 0){
		no_directory_message();
		return;
	}
	
	for(size_t i = 0; i < argc; ++i){
		pid_t pid = fork();
		if(pid < 0){
			printf("Error : fork failed!\n");
		}else if(pid == 0){
			char *dirname = args[i];
			if(!directory_exists(dirname)){
				printf("Error : Directory %s not found\n", dirname);
			}else if(is_dir_empty(dirname) == 0){
				printf("Error : Directory %s is not empty\n", dirname);
			}else{
				if(rmdir(dirname) == 0){
					write_log("Directory %s deleted successfully.", dirname);
				}else{
					printf("Error : Could not delete the directory %s\n", dirname);
				}
			}
			exit(0);
		}else{
			wait(NULL);
		}
	}
}

void list_dir(char *args[], size_t argc){
	if(argc == 0){
		no_directory_message();
		return;
	}
	
	for(size_t i = 0; i < argc; ++i){
		pid_t pid = fork();
	
		if(pid < 0){
			printf("Error : fork failed!\n");
		}else if(pid == 0){
			char * dirname = args[i];
			if(!directory_exists(dirname)){
				printf("Error : Directory %s not found\n", dirname);
			}else if(is_dir_empty(dirname) != 0){
				printf("Directory %s is empty\n", dirname);
			}else{
				struct dirent *entry;
				DIR *dir = opendir(dirname);
				if(dir == NULL){
					printf("Could not read the directory %s\n", dirname);
					exit(1);
				}
				while((entry = readdir(dir)) != NULL){
					printf("%s\n", entry->d_name);
				}
				write_log("Entries of %s is displayed.", dirname);
				closedir(dir);
				exit(0);
			}
		}else{
			wait(NULL);
		}
	}

}

void list_dir_by_extension(char *args[], size_t argc){
	if((argc % 2) != 0){
		printf("Error : Invalid number of arguments\n");
		return;
	}
	
	for(size_t i = 0; i < argc; i += 2){
		pid_t pid = fork();
		
		if(pid < 0){
			printf("Error : Fork failed\n");
		}else if(pid == 0){
			char* path = args[i];
			char *target_extension = args[i + 1];
			if(!directory_exists(path)){
				printf("Error : Directory %s not found\n", path);
			}else if(is_dir_empty(path) != 0){
				printf("Directory %s is not empty\n", path);
			}else{
				struct dirent *entry;
				DIR *dir = opendir(path);	
				if(dir == NULL){
					printf("Could not read the directory %s\n", path);
					exit(1);
				}
				int count = 0;
				while((entry = readdir(dir)) != NULL){
					char *entry_extension = get_file_extension(entry->d_name);
					if(entry_extension && strcmp(entry_extension, target_extension) == 0){
						printf("%s\n", entry->d_name);
						count++;
					}
				}
				if(count == 0){
					printf("Error : No files with the extension %s found in %s\n", target_extension, path);
				}else{
					write_log("Entries with the extension %s of %s is displayed.", target_extension, path);
				}
				closedir(dir);
				exit(0);	 
			}
		}else{
			wait(NULL);
		}
	}
}

void no_directory_message(){
	printf("Error : Directory name is not provided\n");
}

int directory_exists(char *path){
	struct stat statbuf;
	return (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) ? 1 : 0;
}

int is_dir_empty(char *path){
	struct dirent* entry;
	int count = 0;
	
	DIR *dir = opendir(path);
	if(!dir){return -1;}
	
	while((entry = readdir(dir)) != NULL){
		if(++count > 2){
			closedir(dir);
			return 0;
		}
	}
	closedir(dir);
	return 1;
}

char *get_file_extension(char *filename){
	char *dot = strrchr(filename, '.');
	if(!dot || dot == filename){
		return NULL;
	}
	return dot;
}


