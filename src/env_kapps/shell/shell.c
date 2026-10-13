#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "shell.h"


#define MAXARGS 40
#define DEBUG_ELF 1

void printerro(int eno);

void do_help(int argc, char *argv[]);

const cmd_entry_t g_cmd_table[] = {
//    {"cat",         1,  1, &do_cat,         "Display contents of <file>"},
//    {"cd",          1,  1, &do_cd,          "Change directory <dir>"},
//    {"cp",          2,  2, &do_copyfile,    "Copy file <src> <dest>"},
//    {"dump",        2,  2, &do_dump,        "Dump memory <from> <count>" },
//    {"exit",        0,  0, &do_exit,        "Exit system in a clean way.." },
//    {"loadmem",	    1,  2, &do_loadmem,	    "Load a file to the specified memory location <file> <mem>" },
//    {"ls",          0,  1, &do_ls,	        "List directory contents" },
//    {"mkdir",	    1,  1, &do_mkdir_shel,	    "Create the specified folder <dir>" },
//    {"mv",          2,  2, &do_rename_shel,      "Rename file <src> to <dest>" },
//    {"rm",          1,  1, &do_delete,	    "Delete file <file>" },
//    {"rmdir",	    1,  1, &do_rmdir,	    "Delete the specified folder <dir>" },
//    {"run",         1,  1, &do_run,         "Run code at address <addr>" },
//    {"save",        3,  3, &do_save,        "Saves file from <ADDR> to disk save <filename> <filesize>" },
//    {"shst",        0,  0, &do_shst,        "Show systemtick" },
//    {"time",        0,  6, &do_time,        "Display or set the current time and date (time ? for help)" },
//    {"uptime",      0,  0, &do_uptime,      "Display the time the system has been running" },
//    {"writemem",    2,  0, &do_writemem,    "Write memory <addr> [byte ...]" },
//    {"writemem1",   2,  0, &do_writemem1,   "Write a memory location" },
//    {"int2on",      0,  0, &do_int2ON,      "Turn on int3" },
//    {"ascii",       0,  0, &do_ascii,       "Print hex code of keyboard key" },
//    {"recv",        0,  0, &do_receiver,    "Receiver a file and save to disk" },

    {"help",	    0,  0, &do_help,	    "Show available commands" },

    {0, 0, 0, 0, 0 }
};

int process_command_executable( int argc, char *argv[]){
    return 0;
}

int process_command_builtin( int argc, char *argv[]){
    const cmd_entry_t *cmd;
//    FRESULT fr;
#ifdef DEBUG_ELF  
    printf("process_command_builtin\n");
#endif
//    if (argc == 1 && argv[0][1] == ':')
//    {
//        // Change current drive
//        fr = f_chdrive(argv[0]);
//        if (fr)
//        {
//        	printerro(fr);
//        	printf("Drive %s not found\n", argv[0]);
//        }
//        return 1;
//    }
//    else
    {
        // built-in commands
        for (cmd = g_cmd_table; cmd->name; cmd++)
        {
            if (!strcasecmp(argv[0], cmd->name))
            {
                if ((argc - 1) >= cmd->min_args && (cmd->max_args == 0 || (argc - 1) <= cmd->max_args))
                {
                    cmd->function(argc - 1, argv + 1);
                }
                else
                {
                    if (cmd->min_args == cmd->max_args)
                        printf("%s: requires %d argument%s\n", argv[0], cmd->min_args, cmd->min_args == 1 ? "" : "s");
                    else
                        printf("%s: requires %d to %d arguments\n", argv[0], cmd->min_args, cmd->max_args);
                }
                return 1;
            }
        }
    }
    return 0;
}

void process_command(int argc, char *argv[])
{
    if (argc > 0)
    {
        printf("argc[%d]  ",argc);
        for(int i=0;i < argc;i++){
           printf("i[%d]--->%s | ",i,argv[i]);     
        }
        if (process_command_builtin(argc, argv)){
            return;
        }else if (process_command_executable(argc, argv)){
            return;
        }else{
            printf("Unknown command: [%s] Try 'help'.\n", argv[0]);
        }
    }
}

int execute_cmd(char *linebuffer){
    char *args[MAXARGS+1];
    char *ptr;
    int numargs;

    numargs = 0;
    ptr = linebuffer;
    if( linebuffer[0] == 0x1b){
        return 0;
    }
    while (1){
        if (numargs == MAXARGS){
            printf("Limiting to %d arguments.\n", numargs);
            *ptr = 0;
        }
        if (*ptr == 0){
            // end of string
            args[numargs] = 0;
            break;
        }
        while (isspace(*ptr))
            ptr++;
        if (!isspace(*ptr)){
            args[numargs++] = ptr;

            while (*ptr && !isspace(*ptr))
                ptr++;

            if (*ptr == 0)
                continue;

            while (isspace(*ptr)){
                *ptr=0;
                ptr++;
            }
        }
    }
    process_command(numargs,args);
    return 1;
}
 
extern void history_add(const char *cmd);
extern void readline_with_history(char *buf);

int main(int argc, char *argv[]){
    char buffer[128];
    printf("\nSHELL V1 october/13 2026\n");
    int ret;
    while(1){    
        //display_prompt();
        printf(">");
        fflush(stdout);
        readline_with_history(buffer);
        history_add(buffer); 
        ret = execute_cmd(buffer);
        if(ret == 0){
            return 0;
        }
    }
    return 0;
}
