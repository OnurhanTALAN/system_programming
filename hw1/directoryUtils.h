#include <stddef.h>
#ifndef DIRECTORYUTILS_H
#define DIRECTORYUTILS_H

void create_dir(char *args[], size_t argc);
void delete_dir(char *args[], size_t argc);
void list_dir(char *args[], size_t argc);
void list_dir_by_extension(char *args[], size_t argc);

int directory_exists(char *path);
int is_dir_empty(char *path);
void no_directory_message();
char *get_file_extension(char *filename);

#endif
