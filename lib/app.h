
#ifndef LIBPAS_APP_H
#define LIBPAS_APP_H

#include <stdio.h>
#include <stdlib.h>
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

#include "common.h"
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

#define DEF_SERVER_I1LIST I1 i1_arr[request.data_rows_count];I1List i1l;i1l.item=i1_arr;i1l.max_length=request.data_rows_count;i1l.length=0;
#define DEF_SERVER_I2LIST I2 i2_arr[request.data_rows_count];I2List i2l;i2l.item=i2_arr;i2l.max_length=request.data_rows_count;i2l.length=0;
#define DEF_SERVER_I1F1LIST I1F1 i1f1_arr[request.data_rows_count];I1F1List i1f1l;i1f1l.item=i1f1_arr;i1f1l.max_length=request.data_rows_count;i1f1l.length=0;
#define DEF_SERVER_S1LIST(str_sz) S1 s1_arr[request.data_rows_count * str_sz];S1List s1l;s1l.item=s1_arr;s1l.max_length=request.data_rows_count * str_sz;s1l.length=0;
#define DEF_SERVER_S2LIST S2 s2_arr[request.data_rows_count];S2List s2l;s2l.item=s2_arr;s2l.max_length=request.data_rows_count;s2l.length=0;
#define DEF_SERVER_I1S1LIST I1S1 i1s1_arr[request.data_rows_count];I1S1List i1s1l;i1s1l.item=i1s1_arr;i1s1l.max_length=request.data_rows_count;i1s1l.length=0;

#define SERVER_PARSE_I1LIST acp_requestDataToI1List(&request, &i1l);if (i1l.length <= 0) {return;}
#define SERVER_PARSE_I1F1LIST acp_requestDataToI1F1List(&request, &i1f1l);if (i1f1l.length <= 0) {return;}
#define SERVER_PARSE_I2LIST acp_requestDataToI2List(&request, &i2l);if (i2l.length <= 0) {return;}
#define SERVER_PARSE_I1S1LIST acp_requestDataToI1S1List(&request, &i1s1l);if (i1s1l.length <= 0) {return;}

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

#define FORL for (size_t i = 0; i < list->length; i++) 
#define FORLi FORL
#define FORMLi for (size_t i = 0; i < list->max_length; i++) 
#define FORLISTP(V, I) for (size_t I = 0; I < (V)->length; I++) 
#define FORLISTN(V, I) for (size_t I = 0; I < (V).length; I++) 
#define FORLIST(I) for (size_t I = 0; I < list->length; I++) 
#define FORLLj  for (size_t j = i + 1; j < list->length; j++) 
#define FORLISTPL(V, I, J)  for (size_t J = i + 1; J < (V)->length; J++) 
#define LIi list->item[i]
#define LIj list->item[j]
#define LIll list->item[list->length]
#define Lil list->length-1
#define LL list->length
#define LML list->max_length
#define LIiei(v) LIi.v=atoi(v)
#define LIief(v) LIi.v=atof(v)

#define FUN_LOCK(T) int lock ## T (T *item) {if (item == NULL) {return 0;} if (pthread_mutex_lock(&(item->mutex.self)) != 0) {return 0;}return 1;}
#define FUN_TRYLOCK(T) int tryLock ## T (T  *item) {if (item == NULL) {return 0;} if (pthread_mutex_trylock(&(item->mutex.self)) != 0) {return 0;}return 1;}
#define FUN_UNLOCK(T) int unlock ## T (T *item) {if (item == NULL) {return 0;} if (pthread_mutex_unlock(&(item->mutex.self)) != 0) {return 0;}return 1;}

#define DEC_FUN_LOCK(T) extern int lock ## T (T *item);
#define DEC_FUN_TRYLOCK(T) extern int tryLock ## T (T  *item);
#define DEC_FUN_UNLOCK(T) extern int unlock ## T (T *item);

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
#define MUTEX_INITIALIZER {.self=PTHREAD_MUTEX_INITIALIZER, .created = 0, .attr_initialized = 0}
//#define MUTEX pthread_mutex_t
//#define DEF_MUTEX(V) pthread_mutex_t V = PTHREAD_MUTEX_INITIALIZER;
//#define IF_LOCK_MUTEX(P) if(pthread_mutex_lock(P) != 0)
//#define IF_TRYLOCK_MUTEX(P) if(pthread_mutex_trylock(P) != 0)
//#define UNLOCK_MUTEX(P) pthread_mutex_unlock(P)


extern char * strcpyma(char **dest, char *src);

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

extern int createMThread(pthread_t *new_thread, void *(*thread_routine) (void *), void * data);

extern int threadCancelDisable(int *old_state) ;

extern int threadSetCancelState(int state);


#endif 

