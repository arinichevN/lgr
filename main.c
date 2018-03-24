#include "main.h"

int app_state = APP_INIT;

char *db_data_path;
char *db_public_path;
TSVresult config_tsv = TSVRESULT_INITIALIZER;


int sock_port = -1;
int sock_fd = -1;

Peer peer_client = {.fd = &sock_fd, .addr_size = sizeof peer_client.addr};
struct timespec cycle_duration = {0, 0};
char *db_conninfo_log;
PGconn *db_conn_log = NULL;

Mutex progl_mutex = MUTEX_INITIALIZER;
Mutex db_log_mutex = MUTEX_INITIALIZER;

PeerList peer_list;
ProgList prog_list = {NULL, NULL, 0};

#include "util.c"
#include "db.c"

int readSettings(int *port, struct timespec *cd,  char ** db_data_path, char ** db_public_path, char ** db_conninfo_log, TSVresult *config_tsv, const char *data_path) {
    if (!TSVinit(config_tsv, data_path)) {
        TSVclear(config_tsv);
        return 0;
    }
    int _port = TSVgetis(config_tsv, 0, "port");
    int _cd_sec = TSVgetis(config_tsv, 0, "cd_sec");
    int _cd_nsec = TSVgetis(config_tsv, 0, "cd_nsec");
    char *_db_data_path = TSVgetvalues(config_tsv, 0, "db_data_path");
    char *_db_public_path = TSVgetvalues(config_tsv, 0, "db_public_path");
    char *_db_conninfo_log = TSVgetvalues(config_tsv, 0, "db_conninfo_log");
    if (TSVnullreturned(config_tsv)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s: bad row format\n", F);
#endif
        return 0;
    }
    *port = _port;
    cd->tv_sec = _cd_sec;
    cd->tv_nsec = _cd_nsec;
    *db_data_path = _db_data_path;
    *db_public_path = _db_public_path;
    *db_conninfo_log = _db_conninfo_log;
    return 1;
}

int initData() {
    if (!dbp_init(&db_conn_log, db_conninfo_log)) {
        return 0;
    }
    if (!config_getPeerList(&peer_list, NULL, db_public_path)) {
        dbp_free(db_conn_log);
        return 0;
    }
    if (!loadActiveProg(&prog_list, &peer_list, db_data_path)) {
        freeProgList(&prog_list);
        freePeerList(&peer_list);
        dbp_free(db_conn_log);
        return 0;
    }
    return 1;
}

void initApp() {
    if (!readSettings(&sock_port, &cycle_duration, &db_data_path, &db_public_path, &db_conninfo_log, &config_tsv, CONFIG_FILE)) {
        exit_nicely_e("initApp: failed to read settings\n");
    }
#ifdef MODE_DEBUG
    printf("\n\tsock_port: %d, \n\tcycle_duration: %ld sec %ld nsec, \n\tdb_data_path: %s, \n\tdb_public_path: %s, \n\tdb_conninfo_log: %s\n", sock_port, cycle_duration.tv_sec, cycle_duration.tv_nsec, db_data_path, db_public_path, db_conninfo_log);
#endif
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
    DEF_SERVER_I1LIST
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
                        db_saveTableFieldInt("prog", "enable", item->id, 1, NULL, db_data_path);
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
                        db_saveTableFieldInt("prog", "enable", item->id, 0, NULL, db_data_path);
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
            item->state = ACT;
            break;
        case ACT:
            if (ton_ts(item->interval_min, &item->tmr)) {
                if (strcmp(item->kind, LOG_KIND_FTS) == 0) {
                    readFTS(&item->sensor_fts);
                    if (lockMutex(&db_log_mutex)) {
                        pp_saveFTS(item, db_conn_log);
                        unlockMutex(&db_log_mutex);
                    }
                } else {
#ifdef MODE_DEBUG
                    fprintf(stderr, "%s(): unknown kind",F);
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
    printf("%s(): id=%d state=%s time_rest=%ld sec\n",F, item->id, state, tm_rest.tv_sec);
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
    freePeerList(&peer_list);
    dbp_free(db_conn_log);
}

void freeApp() {
    freeData();
    freeSocketFd(&sock_fd);
    freeMutex(&progl_mutex);
    freeMutex(&db_log_mutex);
    TSVclear(&config_tsv);
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
        printf("%s(): %s %d\n", F, getAppState(app_state), data_initialized);
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