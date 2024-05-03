#include "client_registry.h"
#include "globals.h"
#include <pthread.h>

typedef struct client_registry {
    CLIENT **clients;  // Array of client pointers
    int capacity;      // Maximum number of clients
    int count;         // Current number of clients
    pthread_mutex_t mutex;  // Mutex for thread-safe access
} CLIENT_REGISTRY;

CLIENT_REGISTRY *creg_init() {
    CLIENT_REGISTRY *cr = malloc(sizeof(CLIENT_REGISTRY));
    if (cr == NULL) return NULL;

    cr->clients = calloc(MAX_CLIENTS, sizeof(CLIENT *));
    if (cr->clients == NULL) {
        free(cr);
        return NULL;
    }

    cr->capacity = MAX_CLIENTS;
    cr->count = 0;
    pthread_mutex_init(&cr->mutex, NULL);

    return cr;
}

void creg_fini(CLIENT_REGISTRY *cr) {
    pthread_mutex_lock(&cr->mutex);
    for (int i = 0; i < cr->capacity; i++) {
        if (cr->clients[i] != NULL) {
            client_unref(cr->clients[i], "Removing from registry");
            cr->clients[i] = NULL;
        }
    }
    pthread_mutex_unlock(&cr->mutex);

    free(cr->clients);
    pthread_mutex_destroy(&cr->mutex);
    free(cr);
}

CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd) {
    pthread_mutex_lock(&cr->mutex);
    if (cr->count >= cr->capacity) {
        pthread_mutex_unlock(&cr->mutex);
        return NULL;  // Registry is full
    }

    CLIENT *client = client_create(client_registry,fd);
    if (client == NULL) {
        pthread_mutex_unlock(&cr->mutex);
        return NULL;
    }

    for (int i = 0; i < cr->capacity; i++) {
        if (cr->clients[i] == NULL) {
            cr->clients[i] = client;
            cr->count++;
            client_ref(client, "Added to registry");
            pthread_mutex_unlock(&cr->mutex);
            return client;
        }
    }

    pthread_mutex_unlock(&cr->mutex);
    return NULL;  // Should not reach here
}

int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client) {
    pthread_mutex_lock(&cr->mutex);
    for (int i = 0; i < cr->capacity; i++) {
        if (cr->clients[i] == client) {
            cr->clients[i] = NULL;
            cr->count--;
            client_unref(client, "Removed from registry");
            pthread_mutex_unlock(&cr->mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&cr->mutex);
    return -1;  // Client not found
}

CLIENT **creg_all_clients(CLIENT_REGISTRY *cr) {
    pthread_mutex_lock(&cr->mutex);

    // Allocate memory for the maximum number of clients plus one for the NULL terminator
    CLIENT **client_list = malloc((cr->capacity + 1) * sizeof(CLIENT *));
    if (client_list == NULL) {
        pthread_mutex_unlock(&cr->mutex);
        return NULL;  // Memory allocation failed
    }

    int index = 0;
    for (int i = 0; i < cr->capacity; i++) {
        if (cr->clients[i] != NULL) {
            client_list[index] = cr->clients[i];
            client_ref(client_list[index], "Adding to client list");  // Increment ref count
            index++;
        }
    }
    client_list[index] = NULL;  // Terminate the list

    pthread_mutex_unlock(&cr->mutex);
    return client_list;
}

void creg_shutdown_all(CLIENT_REGISTRY *cr) {
    pthread_mutex_lock(&cr->mutex);
    for (int i = 0; i < cr->capacity; i++) {
        if (cr->clients[i] != NULL) {
            shutdown(client_get_fd(cr->clients[i]), SHUT_RDWR);
            client_unref(cr->clients[i], "Shutdown all clients");
            cr->clients[i] = NULL;
        }
    }
    cr->count = 0;
    pthread_mutex_unlock(&cr->mutex);
    // The thread should wait until all clients are effectively unregistered
    // which might be handled by individual client service threads
}
