#include "csapp.h"
#include "user.h"
#include "user_registry.h"
#include "debug.h"

#define INITIAL_SIZE 100

typedef struct user_registry{
    USER **users;
    int size;
    pthread_mutex_t lock;
} USER_REGISTRY;

unsigned int hash(char *str){
    unsigned long hash = 5381;
    int c;
    while((c=*str++)){
        hash = ((hash<<5)+hash)+c;
    }
    return hash%INITIAL_SIZE;
}

USER_REGISTRY *ureg_init(void){
    USER_REGISTRY *ureg = malloc(sizeof(USER_REGISTRY));
    if(!ureg) return NULL;
    ureg->users = calloc(INITIAL_SIZE, sizeof(USER *));
    if(!ureg->users){
        free(ureg);
        return NULL;
    }
    ureg->size = INITIAL_SIZE;
    pthread_mutex_init(&ureg->lock,NULL);
    return ureg;
}

void ureg_fini(USER_REGISTRY *ureg){
    pthread_mutex_lock(&ureg->lock);
    for(int i = 0; i<ureg->size; i++){
        if(ureg->users[i]){
            user_unref(ureg->users[i],"Removing from registry on shutdown");
        }
    }
    Free(ureg->users);
    pthread_mutex_unlock(&ureg->lock);
    pthread_mutex_destroy(&ureg->lock);
    Free(ureg);
}

USER *ureg_register(USER_REGISTRY *ureg, char *handle){
    pthread_mutex_lock(&ureg->lock);
    int index = hash(handle);
    USER *user = ureg->users[index];

    if(user && strcmp(user_get_handle(user),handle)==0){
        user_ref(user, "Re-registering user");
        pthread_mutex_unlock(&ureg->lock);
        return user;
    }

    user = user_create(handle);
    if(!user){
        pthread_mutex_unlock(&ureg->lock);
        return NULL;
    }

    user_ref(user, "Registering new user");
    ureg->users[index]=user;
    pthread_mutex_unlock(&ureg->lock);
    return user;
}

void ureg_unregister(USER_REGISTRY *ureg, char *handle){
    pthread_mutex_lock(&ureg->lock);
    int index = hash(handle);
    USER *user = ureg->users[index];

    if(user&&strcmp(user_get_handle(user),handle)==0){
        user_unref(user,"Unregistering user");
        ureg->users[index]=NULL;
    }
    pthread_mutex_unlock(&ureg->lock);
}


