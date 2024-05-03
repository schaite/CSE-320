#include "client_registry.h"
#include "client.h"
#include "user.h"
#include "user_registry.h"
#include "debug.h"
#include "globals.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct client {
    int fd;
    USER *user;
    MAILBOX *mailbox;
    pthread_mutex_t mutex;
    int ref_count;
} CLIENT;

CLIENT *client_create(CLIENT_REGISTRY *creg, int fd){
    CLIENT *client = malloc(sizeof(CLIENT));
    if(client==NULL){
        debug("Unable to malloc space for client");
        return NULL;
    }
    client->fd = fd;
    client->user = NULL;
    client->mailbox = NULL;
    client->ref_count = 1;

    if(pthread_mutex_init(&client->mutex,NULL)!=0){
        debug("unable to init thread for client");
        return NULL;
    }

    if(creg_register(creg,fd)==NULL){
        debug("unable to register client");
        return NULL;
    }

    return client;
}
CLIENT *client_ref(CLIENT *client, char *why){
    pthread_mutex_lock(&client->mutex);
    client->ref_count++;
    printf("Ref increased (%s): %d\n",why,client->ref_count);
    pthread_mutex_unlock(&client->mutex);
    return client;
}
void client_unref(CLIENT *client, char *why){
    pthread_mutex_lock(&client->mutex);
    client->ref_count--;
    printf("Ref decreased (%s): %d\n",why,client->ref_count);
    if(client->ref_count==0){
        pthread_mutex_unlock(&client->mutex); // Unlock before freeing
        pthread_mutex_destroy(&client->mutex);
        if (client->user) user_unref(client->user, "unref by client_unref");
        if (client->mailbox) mb_unref(client->mailbox, "unref by client_unref");
        free(client);
        return;
    }
    pthread_mutex_unlock(&client->mutex);
}

int client_login(CLIENT *client, char *handle) {
    if (client == NULL || handle == NULL) return -1;

    pthread_mutex_lock(&client->mutex);
    if (client->user != NULL) {
        pthread_mutex_unlock(&client->mutex);
        return -1;  // Already logged in
    }

    USER *user = ureg_register(user_registry, handle);
    if (user == NULL) {
        pthread_mutex_unlock(&client->mutex);
        return -1;  // Handle already in use or other failure
    }

    MAILBOX *mailbox = mb_init(handle);
    if (mailbox == NULL) {
        user_unref(user, "Failed to create mailbox during login");
        pthread_mutex_unlock(&client->mutex);
        return -1;
    }

    client->user = user;
    client->mailbox = mailbox;
    pthread_mutex_unlock(&client->mutex);
    return 0;
}

int client_logout(CLIENT *client) {
    if (client == NULL) {
        return -1;  // Invalid client pointer
    }

    pthread_mutex_lock(&client->mutex);
    if (client->user == NULL) {
        pthread_mutex_unlock(&client->mutex);
        return -1; // Client is not logged in
    }

    // Shutdown and unreference the mailbox
    mb_shutdown(client->mailbox);
    mb_unref(client->mailbox, "Client logout");
    client->mailbox = NULL;

    // Unreference the user
    user_unref(client->user, "Client logout");
    client->user = NULL;

    pthread_mutex_unlock(&client->mutex);
    return 0;
}

USER *client_get_user(CLIENT *client, int no_ref){
    pthread_mutex_lock(&client->mutex);
    if (client->user == NULL) {
        pthread_mutex_unlock(&client->mutex);
        return NULL; // Client is not logged in
    }
    USER *user = client->user;
    if (!no_ref) {
        user_ref(user, "Get user from client");
    }
    pthread_mutex_unlock(&client->mutex);
    return user;
}
MAILBOX *client_get_mailbox(CLIENT *client, int no_ref){
    pthread_mutex_lock(&client->mutex);
    if (client->mailbox == NULL) {
        pthread_mutex_unlock(&client->mutex);
        return NULL; // Client is not logged in
    }
    MAILBOX *mailbox = client->mailbox;
    if (!no_ref) {
        mb_ref(mailbox, "Get mailbox from client");
    }
    pthread_mutex_unlock(&client->mutex);
    return mailbox;
}

int client_get_fd(CLIENT *client){
    pthread_mutex_lock(&client->mutex);
    int fd = client->fd;
    pthread_mutex_unlock(&client->mutex);
    return fd;
}
int client_send_packet(CLIENT *client, CHLA_PACKET_HEADER *pkt, void *data) {
    pthread_mutex_lock(&client->mutex);
    if (proto_send_packet(client->fd, pkt, data) == -1) {
        pthread_mutex_unlock(&client->mutex);
        return -1; // Transmission failure
    }
    pthread_mutex_unlock(&client->mutex);
    return 0;
}

int client_send_ack(CLIENT *client, uint32_t msgid, void *data, size_t datalen) {
    CHLA_PACKET_HEADER pkt;
    pkt.type = CHLA_ACK_PKT;
    pkt.msgid = htonl(msgid);
    pkt.payload_length = htonl(datalen);
    pkt.timestamp_sec = htonl(time(NULL)); // Assume this fills the correct field
    pkt.timestamp_nsec = 0; // No need for nanosecond precision

    return client_send_packet(client, &pkt, data);
}

int client_send_nack(CLIENT *client, uint32_t msgid) {
    CHLA_PACKET_HEADER pkt;
    pkt.type = CHLA_NACK_PKT;
    pkt.msgid = htonl(msgid);
    pkt.payload_length = htonl(0); // NACK packets generally do not have a payload
    pkt.timestamp_sec = htonl(time(NULL)); // Assume this fills the correct field
    pkt.timestamp_nsec = 0; // No need for nanosecond precision

    return client_send_packet(client, &pkt, NULL);
}






