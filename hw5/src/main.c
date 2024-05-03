#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "debug.h"
#include "server.h"
#include "globals.h"
#include "csapp.h"

static void terminate(int);

void handler(int num){
    terminate(EXIT_SUCCESS);
}

/*
 * "Charla" chat server.
 *
 * Usage: charla <port>
 */
int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    int p_flag = 0, i= 1;
    char* port = NULL;

    for(; i<argc; i++){
        if((strcmp(argv[i],"-p")==0)&&(i+1<argc)){
            p_flag = 1;
            port = argv[i+1];
            break;
        }
    }

    if(port == NULL){
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        exit(EXIT_FAILURE);  // Terminate if port is not specified
    }
    if(p_flag == 0||(p_flag = 1 && i == argc)){
        fprintf(stderr, "Usage: %s -p <port>\n",argv[0]);
        terminate(EXIT_FAILURE);
    }

    if(argc != 3){
        fprintf(stderr, "Usage: %s -p <port>\n",argv[0]);
        terminate(EXIT_FAILURE);
    }

    // Perform required initializations of the client_registry and
    // player_registry.
    user_registry = ureg_init();
    client_registry = creg_init();

    //Installing SIGHUP handler using sigaction
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if(sigaction(SIGHUP, &action, &old_action)<0){
        terminate(EXIT_FAILURE);
    }

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function charla_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.
    
    int listenfd, *connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    listenfd = open_listenfd(argv[i+1]);

    while(1){
        clientlen = (sizeof(struct sockaddr_storage));
        connfd = malloc(sizeof(int));
        memset(connfd, 0, sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        if(*connfd<0){
            free(connfd);
            terminate(EXIT_FAILURE);
        }
        pthread_t tid;
        pthread_create(&tid, NULL, chla_client_service, connfd);
    }

    fprintf(stderr, "You have to finish implementing main() "
	    "before the server will function.\n");

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shut down all existing client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    // Finalize modules.
    creg_fini(client_registry);
    ureg_fini(user_registry);

    debug("%ld: Server terminating", pthread_self());
    exit(status);
}



