/*
 * Legion: Command-line interface
 */

#include "legion.h"
#include <string.h>
#include <stdbool.h>

void handle_quit(FILE *out){
    fprintf(out, "Exiting.\n");
    sf_fini();
    exit(0);
}

void run_cli(FILE *in, FILE *out)
{
    char line[1024];
    while(1){
        sf_prompt();
        fprintf(out,"legion> ");
        fflush(out);

        if(!fgets(line,sizeof(line), in)){
            if(feof(in)){
                sf_error("End of transmission (Ctrl-D)");
                break;
            }
            else{
                sf_error("Error reading input");
                break;
            }
        }

        line[strcspn(line, "\n")] = 0;
        
        if(strcmp(line,"quit")==0){
            handle_quit(out);
        }
        else{
            fprintf(out,"unknown command: %s\n",line);
            fflush(out);
        }

    }
}
