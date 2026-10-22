/*
1. Parse arguments -> -a -t 
2. for directoies ls . > ls dir1
3. sort  alphabetically or by time
4. print
5. free
*/

#include "lsr.h"

options_t parse_options(int argc, char** argv)
{
    options_t opts;
    opts.show_all = false;
    opts.sort_time = false;

    // Scan all arguments for flags
    for (int i = 0; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            // It's an option arg
            for (int j = 1; argv[i][j] != '\0'; j++)
            {
                if (argv[i][j] == 'a') {
                    opts.show_all = true;
                } else if (argv[i][j] == 't') {
                    opts.sort_time = true;
                } else {
                    // Invalid option
                    printf("my_ls: invalid option -- %c\n", argv[i][j]);
                }
            }
        }
    }
    return opts;
}

/*
- Collect the non-option arguments (file, dir names)
- separate them
- return total num of dirs
- if no paths, "."
*/
int gather_paths(int argc, char** argv, 
                char** non_dirs, int *non_dir_count, 
                char** dirs, int *dir_count)
{
    int total = 0;
    *dir_count = 0;
    *non_dir_count = 0;

    // skip the argv[0] -> ./my_ls
    for (int i = 1; i < argc; i++)
    {
        // Skip the options
        if (argv[i][0] == '-') {
            continue;
        }

        // Try opening it as a dir
        DIR *temp_dir = rigel_opendir(argv[i]);
        if (temp_dir) {
            // it's a valid dir
            closedir(temp_dir);
            dirs[*dir_count] = argv[i];
            (*dir_count)++;
            total++;
        } else {
            // not a dir (file or invalid)
            struct stat st;
            if (lstat(argv[i], &st) == 0) {
                // It's existing
                non_dirs[*non_dir_count] = argv[i];
                (*non_dir_count)++;
                total++;
            } else {
                // Invalid path
                printf("my_ls: cannot access '%s'\n", argv[i]);
            }
        }
    }

    // if no valid, non-option args are provided, default "."
    if (total == 0) {
        dirs[0] = ".";
        *dir_count = 1;
        total = 1;
    }
    
    return total;
}

/*
- open path as dir
- read each entry, readdir()
- if show_hidden == 0, skip '.'
- use lstat() to get st_mtim.tv_sec/nsec
- check is_dir
- return file_list_t, all the entries found
*/
file_list_t* read_directory(const char* path, bool show_hidden) 
{
    file_list_t *flist = malloc(sizeof(file_list_t));
    if (!flist) {
        printf("Malloc failed!\n");
        exit(1);
    }
    flist->count = 0;

    DIR *dir = opendir(path);
    if (!dir) {
        // we cant open the dir
        printf("my_ls: cannot access '%s;\n", path);
        return flist;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // check show_hidden, skip '.'
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }

        // Build full path for lstat
        char fullPath[PATH_MAX];  
        snprintf(fullPath, PATH_MAX, "%s/%s", path, entry->d_name);

        // printf("Full Path: %s\n", fullPath);

        struct stat st;
        if (lstat(fullPath, &st) == 0) {
            // fill file_info_t
            file_info_t info;
            strncpy(info.name, entry->d_name, PATH_MAX - 1);
            info.name[PATH_MAX - 1] = '\0'; // null termination

            info.sec = st.st_mtim.tv_sec;
            info.nsec = st.st_mtim.tv_nsec;
            info.is_dir = S_ISDIR(st.st_mode);

            // add it flist
            if (flist->count < MAX_FILES) {
                flist->files[flist->count] = info;
                flist->count++;
            } else {
                // we exceed MAX_FILES
                printf("Warning: too many files in '%s' (max %d). Some entries skipped\n", path, MAX_FILES);
                break;
            }
        }
    }
    closedir(dir);
    return flist;
}

/*
- Comparator for qsort to sort by modification time (descending)
- if times are equal, fall back to alphabetical ord.
*/
int compare_time(const void* a,  const void* b) 
{
    const file_info_t* fa = (const file_info_t *)a;
    const file_info_t* fb = (const file_info_t *)b;

    // Compare sec
    if (fa->sec != fb->sec) {
        return (fa->sec > fb->sec) ? -1 : 1;
    }
    if (fa->nsec != fb->nsec) {
        return (fa->nsec > fb->nsec) ? -1 : 1;
    }
    // if the time is same, alphabetically
    return strcmp(fa->name, fb->name);
}

/*
- Comparator for qsort to sort by name (lexicographically)
- return <0 if a < b, >0 if a > b, 0 if equal
*/
int compare_alphabetical(const void* a, const void* b)
{
    const file_info_t *fa = (const file_info_t *)a;
    const file_info_t *fb = (const file_info_t *)b;
    return strcmp(fa->name, fb->name);
}

/*
- print each file/dir name in flist on a line
*/
void print_file_list(const file_list_t* flist)
{
    for (int i = 0; i < flist->count; i++)
    {
        printf("%s\n", flist->files[i].name);
    }
}

void print_file_list_debug(const file_list_t *flist) {
    printf("\n=== Debugging read_directory() Output ===\n");
    printf("Total files read: %d\n", flist->count);

    for (int i = 0; i < flist->count; i++) {
        printf("[%d] Name: %s, Sec: %ld, nSec: %ld, IsDir: %s\n", 
            i, 
            flist->files[i].name,
            flist->files[i].sec,
            flist->files[i].nsec,
            flist->files[i].is_dir ? "Yes" : "No");
    }
    printf("===============================================\n\n");
}

int main(int argc, char** argv)
{
    // 1) Parse options
    options_t opts = parse_options(argc, argv);

    // printf("Options: %d, %d\n", opts.sort_time, opts.show_all);

    // Arrays to store dirs and paths
    char* dirs[argc];
    char* non_dirs[argc];
    memset(dirs, 0, sizeof(dirs)); // Init to NULL
    memset(non_dirs, 0, sizeof(non_dirs)); // Init to NULL
    int dir_count = 0;
    int non_dir_count = 0;

    // 2) Gather Paths
    int total = gather_paths(argc, argv, 
        non_dirs, &non_dir_count, 
        dirs, &dir_count);

    (void)total;
    
    // printf("Total: %d\n", total);
    // printf("dirs: %s\n", dirs[0]);
    // printf("Non dirs: %s\n", non_dirs[0]);

    // Sort non_dirs alpahabetically
    if (non_dir_count > 1) {
        // Bubble sort fo non dir array
        for (int i = 0; i < non_dir_count - 1; i++) {
            for (int j = 0; j < non_dir_count - i - 1; j++) {
                if (strcmp(non_dirs[j], non_dirs[j + 1]) > 0) {
                    char* temp = non_dirs[j];
                    non_dirs[j] = non_dirs[j+1];
                    non_dirs[j+1] = temp;
                }
            }
        }
    }
    // Sort dirs alpahabetically
    if (dir_count > 1) {
        // Bubble sort fo non dir array
        for (int i = 0; i < dir_count - 1; i++) {
            for (int j = 0; j < dir_count - i - 1; j++) {
                if (strcmp(dirs[j], dirs[j + 1]) > 0) {
                    char* temp = dirs[j];
                    dirs[j] = dirs[j+1];
                    dirs[j+1] = temp;
                }
            }
        }
    }

    // 3) Print non_dirs first
    // then a blank line and list dirs after
    for (int i = 0; i < non_dir_count; i++) {
        // Just prin the file name (ls -1)
        printf("%s\n", non_dirs[i]);
    }
    if (non_dir_count > 0 && dir_count > 0) {
        printf("\n");
    }

    // 4) For each dir, read, sort, print!
    for (int i = 0; i < dir_count; i++)
    {
        // dir1:
        // file1
        // file2
        // dir2:
        // fileA
        // fileB
        //...
        if (dir_count > 1) {
            printf("%s:\n", dirs[i]);
        }

        // Read the dir
        file_list_t* flist = read_directory(dirs[i], opts.show_all);

        // print_file_list_debug(flist);

        // Sort by time or name
        if (opts.sort_time) {
            qsort(flist->files, flist->count, sizeof(file_info_t), compare_time);
        } else {
            qsort(flist->files, flist->count, sizeof(file_info_t), compare_alphabetical);
        }

        // Print the result
        print_file_list(flist);

        if (i < dir_count  - 1){
            printf("\n");
        }
    }

    return 0;
}
