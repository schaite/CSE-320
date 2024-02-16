#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    //int ret;
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    debug("Options: 0x%x", global_options);
    if(global_options & 1){
        USAGE(*argv, EXIT_SUCCESS);
        return EXIT_SUCCESS;
    }


    if(global_options&(1<<1)){
        if(compress()==-1){
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    if(global_options&(1<<2)){
        if(decompress()==-1){
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    USAGE(*argv,EXIT_FAILURE);
    return EXIT_FAILURE;

}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
