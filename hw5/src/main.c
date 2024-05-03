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
#include "csapp.h"
#include "server.h"
#include "globals.h"

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

static void terminate(int status);
static void sighup_handler();

extern char *optarg;

/*
 * "Charla" chat server.
 *
 * Usage: charla <port>
 */


int main(int argc, char* argv[]){
    int listenfd, *connfdp;
    char* port;
    socklen_t clientlen = sizeof(struct sockaddr_in);;
    struct sockaddr_in clientaddr;
    pthread_t tid;

    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    int option = getopt(argc, argv, "p:");
    if( option == 'p' ){
        port = optarg;
    }

    // Initialize registries
    user_registry = ureg_init();
    client_registry = creg_init();

      // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function charla_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    struct sigaction action, old_action;
    action.sa_handler = sighup_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if( sigaction(SIGHUP, &action, &old_action) < 0 ) {
        debug("sigaction error");
    }

    listenfd = open_listenfd(port);
    if(listenfd<0) {
        debug("open_listenfd error");
    }

    while(1) {
        connfdp = malloc(sizeof(int));
        if(connfdp==NULL) {
            debug("malloc error");
        }
        *connfdp = accept(listenfd, (SA *)&clientaddr, &clientlen);
        if(*connfdp<0) {
            debug("accept error");
        }
        int ret;
        if( (ret = pthread_create(&tid, NULL, chla_client_service, connfdp))!=0 ) {
            debug("pthread_create error");
        }
    }

    fprintf(stderr, "You have to finish implementing main() "
	    "before the PBX server will function.\n");

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

void sighup_handler() {
    terminate(EXIT_SUCCESS); 
}



// #define BACKLOG 10  // Number of pending connections in the queue

// // Function prototypes
// static void terminate(int);
// static void handle_sighup(int);

// /*
//  * Entry point of the server.
//  */
// int main(int argc, char* argv[]) {
//     char portStr[16]; // Buffer for the port string
//     int listenfd, *connfd;
//     socklen_t clientlen = sizeof(struct sockaddr_in);
//     struct sockaddr_in clientaddr;
//     pthread_t tid;

//     // Parse command-line arguments
//     if (argc != 3 || strcmp(argv[1], "-p") != 0) {
//         fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
//         exit(EXIT_FAILURE);
//     }
//     int port = atoi(argv[2]);
//     sprintf(portStr, "%d", port); // Convert the port number to a string

//     // Initialize registries
//     user_registry = ureg_init();
//     client_registry = creg_init();

//     // Set up the server socket
//     listenfd = Open_listenfd(portStr);
//     if (listenfd < 0) {
//         fprintf(stderr, "Failed to open listen socket\n");
//         exit(EXIT_FAILURE);
//     }

//     // Install SIGHUP handler
//     struct sigaction sa;
//     memset(&sa, 0, sizeof(sa));
//     sa.sa_handler = handle_sighup;
//     sigaction(SIGHUP, &sa, NULL);

//     // Accept loop
//     while (1) {
//         connfd = Malloc(sizeof(int));  // Allocate space to store the descriptor
//         *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

//         // Start a new thread to handle the client
//         Pthread_create(&tid, NULL, chla_client_service, connfd);
//     }

//     // Should never reach here
//     terminate(EXIT_SUCCESS);
// }

// /*
//  * Function called to cleanly shut down the server.
//  */
// static void terminate(int status) {
//     creg_shutdown_all(client_registry);
//     creg_fini(client_registry);
//     ureg_fini(user_registry);
//     exit(status);
// }

// /*
//  * Signal handler for SIGHUP.
//  */
// static void handle_sighup(int signum) {
//     terminate(EXIT_SUCCESS);
// }
