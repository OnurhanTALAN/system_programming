#include <stddef.h>
#ifndef FILEUTILS_H
#define FILEUTILS_H


void read_file(char *args[], size_t argc);
void create_file(char *args[], size_t argc);
void append_to_file(char *args[], size_t argc);
void delete_file(char *args[], size_t argc);

int file_exists(char *path);
void no_filename_message();
void show_logs(char *args[], size_t argc);
void write_log(char *format, ...);

#endif
