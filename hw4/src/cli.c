/*
 * Legion: Command-line interface
 */

#include "legion.h"
#include <string.h>

void handle_quit(FILE *out){
    fprintf(out, "Exiting.\n");
    sf_fini();
    exit(0);
}

void run_cli(FILE *in, FILE *out)
{
    char line[1024];
    //sf_init();
    while(1){
        sf_prompt();
        fprintf(out,"legion> ");
        fflush(out);

        if(fgets(line,sizeof(line), in)==NULL) break;

        line[strcspn(line, "\n")] = 0;
        
        if(strcmp(line,"quit")==0){
            handle_quit(out);
            break;
        }
        else{
            fprintf(out,"unknown command: %s\n",line);
        }

    }
    sf_fini();
}


// void handle_help(char **args, int nargs, FILE *out){
//     fprintf(out, "Available commands:\nhelp (0 args) Print this help message\nquit (0 args) Quit the program\nregister (0 args) Register a demon\nunregister (1 args) unregister a daemon \nstatus (1 args) Show the status of a daemon\nstatus-all (0 args) Show the status of all daemons \nstart (1 args) Start a daemon\nstop (1 args) Stop a daemon \nlogrotate (1 args) Rotate log files for a daemon");
// }

// command_map commands[] = {
//     {"quit", handle_quit},
//     {"help",handle_help},
//     {NULL,NULL}
// };


// typedef struct command_map{
//     char *command_name;
//     void (*handler)(char **args, int nargs, FILE *out);
// }command_map;

