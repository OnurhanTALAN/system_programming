#include "utilities.h"
#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

void print_command_manual(){
	printf("Usage: fileManager <command> [arguments]\n");
	printf("Commands:\n");
	printf("createDir \"folderName\"                       -Create a new directory\n");
	printf("createFile \"fileName\"                        -Create a new file\n");
	printf("listDir                                      -List all files in a directory\n");
	printf("listFilesByExtension \"folderName\" \".txt\"     -List files with specific extension\n");
	printf("readFile \"fileName\"                          -Read a file's content\n");
	printf("appendToFile \"fileName\" \"new content\"        -Append content to a file\n");
	printf("deleteFile \"fileName\"                        -Delete a file\n");
	printf("deleteDir \"folderName\"                       -Delete an empty directory\n");
	printf("showlogs                                     -Display operation logs\n");
}

char* get_timeStamp_string(){
	time_t current_time;
	struct tm *time_info;
	time(&current_time);
	time_info = localtime(&current_time);
	static char timeStamp[25];
	strftime(timeStamp, sizeof(timeStamp), "[%Y-%m-%d %H:%M:%S]", time_info);
	return timeStamp;
}

char* concatenate_strings(char *arr[], size_t size){
	size_t total_length = 0;
	for(size_t i = 0; i < size; ++i){
		total_length += strlen(arr[i]);
	}
	total_length += size;
	char *result = (char *)malloc(total_length * sizeof(char));
	if(result == NULL){
		printf("Memory allocation failed!\n");
		return NULL;
	}
	result[0] = '\0';
	
	for(size_t i = 0; i < size; ++i){
		strcat(result, arr[i]);
		if(i < size){
			strcat(result, " ");
		}
	}
	return result;
}
