#include "main.h"

int app_state = APP_INIT;

char db_data_path[LINE_SIZE];
char db_log_path[LINE_SIZE];
char db_public_path[LINE_SIZE];

int sock_port = -1;
int sock_fd = -1;

unsigned int retry_max = 0;
Peer peer_client = {.fd = &sock_fd, .addr_size = sizeof peer_client.addr};
struct timespec cycle_duration = {0, 0};

I1List i1l;

Mutex progl_mutex = MUTEX_INITIALIZER;
Mutex db_log_mutex = MUTEX_INITIALIZER;

PeerList peer_list;
ProgList prog_list = {NULL, NULL, 0};

#include "util.c"
#include "db.c"

int readSettings() {
    FILE* stream = fopen(CONFIG_FILE, "r");
    if (stream == NULL) {
#ifdef MODE_DEBUG
        perror("readSettings()");
#endif
        return 0;
    }
    skipLine(stream);
    int n;
    n = fscanf(stream, "%d\t%ld\t%ld\t%u\t%255s\t%255s\t%255s\n",
            &sock_port,
            &cycle_duration.tv_sec,
            &cycle_duration.tv_nsec,
            &retry_max,
            db_data_path,
            db_public_path,
            db_log_path
            );
    if (n != 7) {
        fclose(stream);
#ifdef MODE_DEBUG
        fputs("ERROR: readSettings: bad row format\n", stderr);
#endif
        return 0;
    }
    fclose(stream);
#ifdef MODE_DEBUG
    printf("readSettings: \n\tsock_port: %d, \n\tcycle_duration: %ld sec %ld nsec, \n\tretry_max: %u, \n\tdb_data_path: %s, \n\tdb_public_path: %s, \n\tdb_log_path: %s\n", sock_port, cycle_duration.tv_sec, cycle_duration.tv_nsec, retry_max, db_data_path, db_public_path, db_log_path);
#endif
    return 1;
}

int initData() {
    if (!initI1List(&i1l, ACP_BUFFER_MAX_SIZE)) {
        return 0;
    }
    if (!config_getPeerList(&peer_list, NULL, db_public_path)) {
        FREE_LIST(&i1l);
        return 0;
    }
    if (!loadActiveProg(&prog_list, &peer_list, db_data_path)) {
        freeProgList(&prog_list);
        FREE_LIST(&peer_list);
        FREE_LIST(&i1l);
        return 0;
    }
    return 1;
}

void initApp() {
    if (!readSettings()) {
        exit_nicely_e("initApp: failed to read settings\n");
    }
    if (!initMutex(&progl_mutex)) {
        exit_nicely_e("initApp: failed to initialize mutex\n");
    }
    if (!initMutex(&db_log_mutex)) {
        exit_nicely_e("initApp: failed to initialize db_log_mutex\n");
    }
    if (!initServer(&sock_fd, sock_port)) {
        exit_nicely_e("initApp: failed to initialize udp server\n");
    }
}

void serverRun(int *state, int init_state) {
    SERVER_HEADER
    SERVER_APP_ACTIONS
    if (
            ACP_CMD_IS(ACP_CMD_PROG_STOP) ||
            ACP_CMD_IS(ACP_CMD_PROG_START) ||
            ACP_CMD_IS(ACP_CMD_PROG_RESET) ||
            ACP_CMD_IS(ACP_CMD_PROG_ENABLE) ||
            ACP_CMD_IS(ACP_CMD_PROG_DISABLE) ||
            ACP_CMD_IS(ACP_CMD_PROG_GET_DATA_INIT) ||
            ACP_CMD_IS(ACP_CMD_PROG_GET_DATA_RUNTIME)
            ) {
        acp_requestDataToI1List(&request, &i1l);
        if (i1l.length <= 0) {
            return;
        }
    } else {
        return;
    }


    if (ACP_CMD_IS(ACP_CMD_PROG_STOP)) {
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                deleteProgById(i1l.item[i], &prog_list, db_data_path);
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_START)) {
        for (int i = 0; i < i1l.length; i++) {
            addProgById(i1l.item[i], &prog_list, &peer_list, NULL, db_data_path);
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_RESET)) {
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                item->state = OFF;
                deleteProgById(i1l.item[i], &prog_list, db_data_path);
            }
        }
        for (int i = 0; i < i1l.length; i++) {
            addProgById(i1l.item[i], &prog_list, &peer_list, NULL, db_data_path);
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_ENABLE)) {
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    if (item->state == OFF) {
                        item->state = INIT;
                        db_saveTableFieldInt("prog","enable",item->id, 1, NULL, db_data_path);
                    }
                    unlockMutex(&item->mutex);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_DISABLE)) {
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    if (item->state != OFF) {
                        item->state = DISABLE;
                        db_saveTableFieldInt("prog","enable",item->id, 0, NULL, db_data_path);
                    }
                    unlockMutex(&item->mutex);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_DATA_INIT)) {
        for (int i = 0; i < i1l.length; i++) {
            Prog *prog = getProgById(i1l.item[i], &prog_list);
            if (prog != NULL) {
                if (!bufCatProgInit(prog, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_DATA_RUNTIME)) {
        for (int i = 0; i < i1l.length; i++) {
            Prog *prog = getProgById(i1l.item[i], &prog_list);
            if (prog != NULL) {
                if (!bufCatProgRuntime(prog, &response)) {
                    return;
                }
            }
        }
    }
    acp_responseSend(&response, &peer_client);
}

void progControl(Prog *item) {
    switch (item->state) {
        case INIT:
            item->tmr.ready = 0;
            item->retry_count = 0;
            item->state = ACT;
            break;
        case ACT:
            if (ton_ts(item->interval_min, &item->tmr) || (item->retry_count > 0 && item->retry_count < retry_max)) {
                if (strcmp(item->kind, LOG_KIND_FTS) == 0) {
                    if (readFTS(&item->sensor_fts)) {
                        if (lockMutex(&db_log_mutex)) {
                            saveFTS(item, db_log_path);
                            unlockMutex(&db_log_mutex);
                        }
                        item->retry_count = 0;
#ifdef MODE_DEBUG
                        printf("saved: id=%d value=%f\n", item->id, item->sensor_fts.value.value);
#endif  
                    } else {
                        item->retry_count++;
#ifdef MODE_DEBUG
                        printf("failed to read: id=%d retry_count=%u\n", item->id, item->retry_count);
#endif  
                    }


                } else {
#ifdef MODE_DEBUG
                    puts("progControl: unknown kind");
#endif
                }
            }
            break;
        case DISABLE:
            item->state = OFF;
            break;
        case OFF:
            break;
        default:
            item->state = INIT;
            break;
    }

#ifdef MODE_DEBUG
    struct timespec tm_rest = getTimeRest_ts(item->interval_min, item->tmr.start);
    char *state = getStateStr(item->state);
    printf("progControl: id=%d state=%s time_rest=%ld sec\n", item->id, state, tm_rest.tv_sec);
#endif
}

void cleanup_handler(void *arg) {
    Prog *item = arg;
    printf("cleaning up thread %d\n", item->id);
}

void *threadFunction(void *arg) {
    Prog *item = arg;
#ifdef MODE_DEBUG
    printf("thread for program with id=%d has been started\n", item->id);
#endif
#ifdef MODE_DEBUG
    pthread_cleanup_push(cleanup_handler, item);
#endif
    while (1) {
        struct timespec t1 = getCurrentTime();
        int old_state;
        if (threadCancelDisable(&old_state)) {
            if (lockMutex(&item->mutex)) {
                progControl(item);
                unlockMutex(&item->mutex);
            }
            threadSetCancelState(old_state);
        }
        sleepRest(item->cycle_duration, t1);
    }
#ifdef MODE_DEBUG
    pthread_cleanup_pop(1);
#endif
}

void freeData() {
    stopAllProgThreads(&prog_list);
    freeProgList(&prog_list);
    FREE_LIST(&peer_list);
    FREE_LIST(&i1l);
}

void freeApp() {
    freeData();
    freeSocketFd(&sock_fd);
    freeMutex(&progl_mutex);
    freeMutex(&db_log_mutex);
}

void exit_nicely() {
    freeApp();
#ifdef MODE_DEBUG
    puts("\nBye...");
#endif
    exit(EXIT_SUCCESS);
}

void exit_nicely_e(char *s) {
    fprintf(stderr, "%s", s);
    freeApp();
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
#ifndef MODE_DEBUG
    daemon(0, 0);
#endif
    conSig(&exit_nicely);
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("main: memory locking failed");
    }
    int data_initialized = 0;
    while (1) {
#ifdef MODE_DEBUG
        printf("main(): %s %d\n", getAppState(app_state), data_initialized);
#endif
        switch (app_state) {
            case APP_INIT:
                initApp();
                app_state = APP_INIT_DATA;
                break;
            case APP_INIT_DATA:
                data_initialized = initData();
                app_state = APP_RUN;
                delayUsIdle(1000000);
                break;
            case APP_RUN:
                serverRun(&app_state, data_initialized);
                break;
            case APP_STOP:
                freeData();
                data_initialized = 0;
                app_state = APP_RUN;
                break;
            case APP_RESET:
                freeApp();
                delayUsIdle(1000000);
                data_initialized = 0;
                app_state = APP_INIT;
                break;
            case APP_EXIT:
                exit_nicely();
                break;
            default:
                exit_nicely_e("main: unknown application state");
                break;
        }
    }
    freeApp();
    return (EXIT_SUCCESS);
}