
#include "app.h"

void putse(const char *str) {
#ifdef MODE_DEBUG
    fputs(str, stderr);
#endif
}

void printfe(const char *str, ...) {
#ifdef MODE_DEBUG
    va_list argp;
    va_start(argp, str);
    fprintf(stderr, str, argp);
    va_end(argp);
#endif
}

int file_exist(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

void setPriorityMax(int policy) {
    int max = sched_get_priority_max(policy);
    if (max == -1) {
        perror("sched_get_priority_max()");
        return;
    }
    struct sched_param sp;
    memset(&sp, 0, sizeof sp);
    sp.__sched_priority = max;
    int ret = sched_setscheduler(0, policy, &sp);
    if (ret == -1) {
        perror("sched_setscheduler()");
    }
}

void conSig(void (*fn)()) {
    signal(SIGINT, fn);
    signal(SIGTSTP, fn);
    signal(SIGTERM, fn);
    signal(SIGPIPE, SIG_IGN);
}

int readHostName(char *hostname) {
    memset(hostname, 0, HOST_NAME_MAX);
    if (gethostname(hostname, HOST_NAME_MAX)) {
        perror("readHostName()");
        return 0;
    }
    return 1;
}

/*
 *call it if you want to prevent many instances of particular process from running at the same time
 */
int initPid(int *pid_file, int *pid, const char *pid_path) {
    *pid_file = creat(pid_path, O_RDWR);
    int rc;
    char pid_str[INT_STR_SIZE];
    int n_written;
    if (*pid_file == -1) {
        fprintf(stderr,"%s: couldn't create pid file\n",__FUNCTION__ );
        return 0;
    } else {
        rc = flock(*pid_file, LOCK_EX | LOCK_NB);
        if (rc) {//lock failed
            if (errno == EWOULDBLOCK) {
                fprintf(stderr,"%s: another instance of this process is running\n", __FUNCTION__);
                return 0;
            } else {
                fprintf(stderr,"%s: lock failed\n", __FUNCTION__);
                return 0;
            }
        } else {//lock succeeded
            __pid_t p;
            p = getpid();
            if (pid != NULL) {
                *pid = (int) p;
#ifdef MODE_DEBUG
                printf("initPid: \n\tPID: %d\n", *pid);
#endif
            }

            snprintf(pid_str,INT_STR_SIZE ,"%d\n", p);
            n_written = write(*pid_file, pid_str, sizeof pid_str);
            if (n_written != sizeof pid_str) {
                fprintf(stderr,"%s: writing to pid file failed\n", __FUNCTION__);
                return 0;
            }
        }
    }
    return 1;
}

char * getAppState(char state) {
    switch (state) {
        case APP_INIT:
            return "APP_INIT";
        case APP_INIT_DATA:
            return "APP_INIT_DATA";
        case APP_RUN:
            return "APP_RUN";
        case APP_STOP:
            return "APP_STOP";
        case APP_RESET:
            return "APP_RESET";
        case APP_EXIT:
            return "APP_EXIT";
    }
    return "";
}

void freePid(int *pid_file, int *pid, const char *pid_path) {
    if (*pid_file != -1) {
        close(*pid_file);
        *pid_file = -1;
    }
    if (*pid >= 0) {
        unlink(pid_path);
        *pid = -1;
    }
}

int initMutex(Mutex *m) {
    m->created = 0;
    m->attr_initialized = 0;
    if (pthread_mutexattr_init(&m->attr) != 0) {
        fprintf(stderr,"%s(): ", __FUNCTION__);perror("pthread_mutexattr_init()");
        return 0;
    }
    m->attr_initialized = 1;
    if (pthread_mutexattr_settype(&m->attr, PTHREAD_MUTEX_ERRORCHECK) != 0) {
        fprintf(stderr,"%s(): ", __FUNCTION__);perror("pthread_mutexattr_settype()");
        return 0;
    }

    if (pthread_mutex_init(&m->self, &m->attr) != 0) {
        fprintf(stderr,"%s(): ", __FUNCTION__);perror("pthread_mutex_init()");
        return 0;
    }
    m->created = 1;
    return 1;
}

void freeMutex(Mutex *m) {
    if (m->attr_initialized) {
        if (pthread_mutexattr_destroy(&m->attr) != 0) {
            fprintf(stderr,"%s(): ", __FUNCTION__);perror("pthread_mutexattr_destroy()");
        } else {
            m->attr_initialized = 0;
        }
    }
    if (m->created) {
        if (pthread_mutex_destroy(&m->self) != 0) {
            fprintf(stderr,"%s(): ", __FUNCTION__);perror("pthread_mutex_destroy()");
        } else {
            m->created = 0;
        }
    }
}

int lockMutex(Mutex *item) {
    if (pthread_mutex_lock(&item->self) != 0) {
        fprintf(stderr,"%s(): ", __FUNCTION__);perror("pthread_mutex_lock()");
        return 0;
    }
    return 1;
}

int tryLockMutex(Mutex *item) {
    if (pthread_mutex_trylock(&item->self) != 0) {
        return 0;
    }
    return 1;
}

int unlockMutex(Mutex *item) {
    if (pthread_mutex_unlock(&item->self) != 0) {
        fprintf(stderr,"%s(): ", __FUNCTION__);perror("pthread_mutex_unlock()");
        return 0;
    }
    return 1;
}

void skipLine(FILE* stream) {
    int x;
    while (1) {
        x = fgetc(stream);
        if (x == EOF || x == '\n') {
            break;
        }
    }
}

int createThread(pthread_t *new_thread, void *(*thread_routine) (void *), char *cmd) {
    *cmd = 0;
    if (pthread_create(new_thread, NULL, thread_routine, (void *) cmd) != 0) {
        fprintf(stderr,"%s(): ", __FUNCTION__);perror("pthread_create()");
        return 0;
    }
    return 1;
}

int createMThread(pthread_t *new_thread, void *(*thread_routine) (void *), void * data) {
    if (pthread_create(new_thread, NULL, thread_routine, data) != 0) {
        fprintf(stderr,"%s(): ", __FUNCTION__);perror("pthread_create()");
        return 0;
    }
    return 1;
}

int threadCancelDisable(int *old_state) {
    int r = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, old_state);
    if (r != 0) {
        fprintf(stderr,"%s(): ", __FUNCTION__);perror("pthread_setcancelstate()");
        return 0;
    }
    return 1;
}

int threadSetCancelState(int state){
    int r=pthread_setcancelstate(state, NULL);
    if (r != 0) {
        fprintf(stderr, "%s(): ", __FUNCTION__);
        perror("pthread_setcancelstate()");
        return 0;
    }
    return 1;
}