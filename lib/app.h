
#ifndef LIBPAS_APP_H
#define LIBPAS_APP_H

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <sched.h>

#include <signal.h>

#include <pthread.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "main.h"

#include "acp/app.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define DEF_THREAD pthread_t thread;char thread_cmd=0;void *threadFunction(void *arg);
#define THREAD_CREATE createThread(&thread,&threadFunction,&thread_cmd)
#define THREAD_STOP thread_cmd = 1;pthread_join(thread, NULL);
#define THREAD_EXIT_ON_CMD if (*cmd) {*cmd = 0;return (EXIT_SUCCESS); }
#define THREAD_DEF_CMD char *cmd = (char *) arg;

#define SERVER_HEADER \
    ACPResponse response;ACPRequest request;\
    acp_requestInit(&request);\
    acp_responseInit(&response);\
    if (!acp_requestRead(&request, &peer_client)) {return;}\
    if (!acp_requestCheck(&request)) {return;}\
    acp_responseCoopRequest(&response, &request);

#define SERVER_APP_ACTIONS \
    if (acp_cmdcmp(&request, ACP_CMD_APP_START)) {\
        if (!init_state) {*state = APP_INIT_DATA;}\
        return;\
    } else if (acp_cmdcmp(&request, ACP_CMD_APP_STOP)) {\
        if (init_state) {*state = APP_STOP;}\
        return;\
    } else if (acp_cmdcmp(&request, ACP_CMD_APP_RESET)) {\
        *state = APP_RESET;\
        return;\
    } else if (acp_cmdcmp(&request, ACP_CMD_APP_EXIT)) {\
        *state = APP_EXIT;\
        return;\
    } else if (acp_cmdcmp(&request, ACP_CMD_APP_PING)) {\
        if (init_state) {acp_responseSendStr(ACP_RESP_APP_BUSY, ACP_LAST_PACK, &response, &peer_client);} else {acp_responseSendStr(ACP_RESP_APP_IDLE, ACP_LAST_PACK, &response, &peer_client);}\
        return;\
    } else if (acp_cmdcmp(&request, ACP_CMD_APP_PRINT)) {\
        printData(&response);\
        return;\
    } else if (acp_cmdcmp(&request, ACP_CMD_APP_HELP)) {\
        printHelp(&response);\
        return;\
    }else if (acp_cmdcmp(&request, ACP_CMD_APP_TIME)) {\
        acp_responseSendCurTime(&response, &peer_client);\
        return;\
    }\
    if (!init_state) {return;}

#define SEND_STR(V) acp_responseSendStr(V, ACP_MIDDLE_PACK, response, &peer_client);
#define SEND_STR_L(V) acp_responseSendStr(V, ACP_LAST_PACK, response, &peer_client);

#define SEND_STR_P(V) acp_responseSendStr(V, ACP_MIDDLE_PACK, &response, &peer_client);
#define SEND_STR_L_P(V) acp_responseSendStr(V, ACP_LAST_PACK, &response, &peer_client);

#define LIST_GET_BY_ID \
     int i;\
    for (i = 0; i < list->length; i++) {\
        if (list->item[i].id == id) {\
            return &(list->item[i]);\
        }\
    }\
    return NULL;
#define LIST_GET_BY_IDSTR \
     int i;\
    for (i = 0; i < list->length; i++) {\
        if (strcmp(list->item[i].id, id)==0) {\
            return &(list->item[i]);\
        }\
    }\
    return NULL;

#define LLIST_GET_BY_ID(T) \
    T *curr = list->top;\
    while(curr!=NULL){\
        if(curr->id==id){\
            return curr;\
        }\
        curr=curr->next;\
    }\
    return NULL;

#define LIST_GET_BY(V) \
     int i;\
    for (i = 0; i < list->length; i++) {\
        if (list->item[i].V == id) {\
            return &(list->item[i]);\
        }\
    }\
    return NULL;

#define FORL for (i = 0; i < list->length; i++) 
#define LIi list->item[i]
#define Lil list->length-1

#define FUN_LOCK(T) int lock ## T (T *item) {if (item == NULL) {return 0;} if (pthread_mutex_lock(&(item->mutex.self)) != 0) {return 0;}return 1;}
#define FUN_TRYLOCK(T) int tryLock ## T (T  *item) {if (item == NULL) {return 0;} if (pthread_mutex_trylock(&(item->mutex.self)) != 0) {return 0;}return 1;}
#define FUN_UNLOCK(T) int unlock ## T (T *item) {if (item == NULL) {return 0;} if (pthread_mutex_unlock(&(item->mutex.self)) != 0) {return 0;}return 1;}

#define DEF_FUN_LOCK(T) extern int lock ## T (T *item);
#define DEF_FUN_TRYLOCK(T) extern int tryLock ## T (T  *item);
#define DEF_FUN_UNLOCK(T) extern int unlock ## T (T *item);

enum {
    APP_INIT = 90,
    APP_INIT_DATA,
    APP_RUN,
    APP_STOP,
    APP_RESET,
    APP_EXIT
} State;

typedef struct {
    pthread_mutex_t self;
    pthread_mutexattr_t attr;
    int created;
    int attr_initialized;
} Mutex;

extern void putse(const char *str);

extern void printfe(const char *str, ...);

extern int file_exist(const char *filename);

extern int readConf(const char *path, char conninfo[LINE_SIZE], char app_class[NAME_SIZE]);

extern void conSig(void (*fn)());

extern void setPriorityMax(int policy);

extern int readHostName(char *hostname);

extern int initPid(int *pid_file, int *pid, const char *pid_path);

extern char * getAppState(char state);

extern void freePid(int *pid_file, int *pid, const char *pid_path);

extern int initMutex(Mutex *m);

extern void freeMutex(Mutex *m);

extern int lockMutex(Mutex *item);

extern int tryLockMutex(Mutex *item);

extern int unlockMutex(Mutex *item);

extern void skipLine(FILE* stream);

extern int createThread(pthread_t *new_thread,void *(*thread_routine) (void *),char *cmd);

#endif 

