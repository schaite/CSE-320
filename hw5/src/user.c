#include "csapp.h"
#include "user.h"
#include "debug.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct user{
    char* handle;
    int ref_count;
    pthread_mutex_t lock;
} USER;

USER *user_create(char *handle){
    USER *user = malloc(sizeof(USER));
    if(user==NULL) return NULL;

    user->handle = strdup(handle);
    if(user->handle == NULL){
        free(user);
        return NULL;
    }

    user->ref_count = 1;
    pthread_mutex_init(&user->lock, NULL);

    return user;
}

USER *user_ref(USER *user, char *why){
    pthread_mutex_lock(&user->lock);
    user->ref_count++;
    debug("Increased ref cound for %s: %s\n", user->handle, why);
    pthread_mutex_unlock(&user->lock);
    return user;
}

void user_unref(USER *user, char *why){
    pthread_mutex_lock(&user->lock);
    user->ref_count--;
    debug("Decreased ref count for %s: %s\n",user->handle,why);
    if(user->ref_count==0){
        pthread_mutex_unlock(&user->lock);
        pthread_mutex_destroy(&user->lock);
        free(user->handle);
        free(user);
        return;
    }
    pthread_mutex_unlock(&user->lock);
}

char *user_get_handle(USER *user){
    return user->handle;
}
