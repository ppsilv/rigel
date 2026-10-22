#ifndef MY_LS_H
#define MY_LS_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <rigel/rigel.h>

// Max files in a dir (adjustable)
#define MAX_FILES 2000
#define PATH_MAX  255
#define MAX_FILES 1024

// Strucuture to hold command-line opritons (-a, -t)
typedef struct s_options {
    bool show_all; // -a
    bool sort_time; // -t
} options_t;

// Structure tp represent a single file or dir entry
// Name, modification (sec, nsec)
typedef struct s_file_info {
    char name[PATH_MAX];
    long sec; // st_mtim.tv_sec
    long nsec; // st_mtim.tv_nsec
    bool is_dir;
} file_info_t;

// A container for an array of file_info_t + a count
typedef struct s_file_list {
    file_info_t files[MAX_FILES];
    int count;
} file_list_t;

// Function declerations
options_t parse_options(int argc, char** argv);
int gather_paths(int argc, char** argv, 
                char** non_dirs, int *non_dir_count, 
                char** dirs, int *dir_count);

file_list_t* read_directory(const char*path, bool show_hidden);

int compare_time(const void* a,  const void* b);
int compare_alphabetical(const void* a, const void* b);

void print_file_list(const file_list_t* flist);
void print_file_list_debug(const file_list_t *flist);

#endif
