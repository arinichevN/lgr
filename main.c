#include "main.h"

int app_state = APP_INIT;

char db_data_path[LINE_SIZE];
char db_log_path[LINE_SIZE];
char db_public_path[LINE_SIZE];

int sock_port = -1;
int sock_fd = -1;
int sock_fd_tf = -1;
unsigned int retry_max = 0;
Peer peer_client = {.fd = &sock_fd, .addr_size = sizeof peer_client.addr};
struct timespec cycle_duration = {0, 0};
DEF_THREAD
        struct timespec rsens_interval_min = {1, 0};
struct timespec peer_ping_interval = {3, 0};
I1List i1l;
Mutex progl_mutex = {.created = 0, .attr_initialized = 0};

PeerList peer_list = {NULL, 0};
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
    if (!config_getPeerList(&peer_list, &sock_fd_tf, db_public_path)) {
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!loadActiveProg(db_data_path, &prog_list, &peer_list)) {
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!initI1List(&i1l, ACP_BUFFER_MAX_SIZE)) {
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!THREAD_CREATE) {
        FREE_LIST(&i1l);
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
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

    if (!initServer(&sock_fd, sock_port)) {
        exit_nicely_e("initApp: failed to initialize udp server\n");
    }

    if (!initClient(&sock_fd_tf, WAIT_RESP_TIMEOUT)) {
        exit_nicely_e("initApp: failed to initialize udp client\n");
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
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                deleteProgById(i1l.item[i], &prog_list, db_data_path);
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_START)) {
        for (int i = 0; i < i1l.length; i++) {
            addProgById(i1l.item[i], &prog_list, &peer_list, db_data_path);
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_RESET)) {
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                curr->state = OFF;
                deleteProgById(i1l.item[i], &prog_list, db_data_path);
            }
        }
        for (int i = 0; i < i1l.length; i++) {
            addProgById(i1l.item[i], &prog_list, &peer_list, db_data_path);
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_ENABLE)) {
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    curr->state = INIT;
                    saveProgEnable(curr->id, 1, db_data_path);
                    unlockProg(curr);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_DISABLE)) {
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    curr->state = DISABLE;
                    saveProgEnable(curr->id, 0, db_data_path);
                    unlockProg(curr);
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

/*


int readFTS(SensorFTS *s) {
    struct timespec now = getCurrentTime();
    s->source->active = 1;
    s->source->time1 = now;
    s->last_read_time = now;
    s->last_return = 1;
    s->value.state = 1;
    s->value.value += 0.001f;
    s->value.tm = now;
    return 1;
}
 */

int readFTS(SensorFTS *s) {
    return acp_readSensorFTS(s);
}

/*
int saveFTS(Prog *item) {
    char q[LINE_SIZE];
    char *status;
    if (item->sensor_fts->value.state) {
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_FAILURE;
    }
    snprintf(q, sizeof q, "select log.do_real(%d, %0.3f, %ld, %ld, '%s')",
            item->id,
            item->sensor_fts.value.value,
            item->max_rows,
            item->sensor_fts.value.tm.tv_sec,
            status
            );
    int n = dbGetDataN(db_conn_log, q, q);
    if (n != 0) {
        return 0;
    }
    return 1;
}
 */

int saveFTS(Prog *item, const char *db_path) {
    if (item->max_rows <= 0) {
        return 0;
    }
    if (!file_exist(db_path)) {
#ifdef MODE_DEBUG
        fputs("saveFTS: file not found\n", stderr);
#endif
        return 0;
    }
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
#ifdef MODE_DEBUG
        fputs("saveFTS: db open failed\n", stderr);
#endif
        return 0;
    }
    int n = 0;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select count(*) from v_real where id=%d", item->id);
    if (!db_getInt(&n, db, q)) {
        putse("saveFTS: db_getInt failed");
        sqlite3_close(db);
        return 0;
    }
    char *status;
    if (item->sensor_fts.value.state) {
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_FAILURE;
    }
    struct timespec now = getCurrentTime();
    if (n < item->max_rows) {
        snprintf(q, sizeof q, "insert into v_real(id, mark, value, status) values (%d, %ld, %f, '%s')", item->id, now.tv_sec, item->sensor_fts.value.value, status);
        if (!db_exec(db, q, 0, 0)) {
#ifdef MODE_DEBUG
            fprintf(stderr, "saveFTS: insert failed\n");
#endif
        }
    } else {
        snprintf(q, sizeof q, "update v_real set mark = %ld, value = %f, status = '%s' where id = %d and mark = (select min(mark) from v_real where id = %d)", now.tv_sec, item->sensor_fts.value.value, status, item->id, item->id);
        if (!db_exec(db, q, 0, 0)) {
            printfe("saveFTS: update failed\n");
            return 0;
        }
    }
    sqlite3_close(db);
    return 0;
}

/*
int clearLog(int dev_id, sqlite3 *db) {
    char q[LINE_SIZE];
    char *err_msg = 0;
    snprintf(q, sizeof q, "delete from v_real where dev_id=%d", dev_id);
    int rc = sqlite3_exec(db, q, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printfe("saveProgActive: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}
 */

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
                        saveFTS(item, db_log_path);
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

void *threadFunction(void *arg) {
    THREAD_DEF_CMD
#ifdef MODE_DEBUG
            puts("threadFunction: running...");
#endif
    while (1) {
        struct timespec t1 = getCurrentTime();
        lockProgList();
        Prog *curr = prog_list.top;
        unlockProgList();
        while (1) {
            if (curr == NULL) {
                break;
            }

            if (tryLockProg(curr)) {
                progControl(curr);
                Prog *temp = curr;
                curr = curr->next;
                unlockProg(temp);
            }

            THREAD_EXIT_ON_CMD
        }
        THREAD_EXIT_ON_CMD
        sleepRest(cycle_duration, t1);
    }
}

void freeProg(ProgList *list) {
    Prog *curr = list->top, *temp;
    while (curr != NULL) {
        temp = curr;
        curr = curr->next;
        free(temp);
    }
    list->top = NULL;
    list->last = NULL;
    list->length = 0;
}

void freeData() {
#ifdef MODE_DEBUG
    puts("freeData:");
#endif
    THREAD_STOP
    FREE_LIST(&i1l);
    freeProg(&prog_list);
    FREE_LIST(&peer_list);
#ifdef MODE_DEBUG
    puts(" done");
#endif
}

void freeApp() {
#ifdef MODE_DEBUG
    puts("freeApp:");
#endif
    freeData();
#ifdef MODE_DEBUG
    puts(" freeData: done");
#endif
    freeSocketFd(&sock_fd);
#ifdef MODE_DEBUG
    puts(" free sock_fd: done");
#endif
    freeSocketFd(&sock_fd_tf);
#ifdef MODE_DEBUG
    puts(" sock_fd_tf: done");
#endif
#ifdef MODE_DEBUG
    puts(" done");
#endif
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
    if (geteuid() != 0) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s: root user expected\n", APP_NAME_STR);
#endif
        return (EXIT_FAILURE);
    }
#ifndef MODE_DEBUG
    daemon(0, 0);
#endif
    conSig(&exit_nicely);
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("main: memory locking failed");
    }
    int data_initialized = 0;
    while (1) {
        switch (app_state) {
            case APP_INIT:
#ifdef MODE_DEBUG
                puts("MAIN: init");
#endif
                initApp();
                app_state = APP_INIT_DATA;
                break;
            case APP_INIT_DATA:
#ifdef MODE_DEBUG
                puts("MAIN: init data");
#endif
                data_initialized = initData();
                app_state = APP_RUN;
                delayUsIdle(1000000);
                break;
            case APP_RUN:
#ifdef MODE_DEBUG
                puts("MAIN: run");
#endif
                serverRun(&app_state, data_initialized);
                break;
            case APP_STOP:
#ifdef MODE_DEBUG
                puts("MAIN: stop");
#endif
                freeData();
                data_initialized = 0;
                app_state = APP_RUN;
                break;
            case APP_RESET:
#ifdef MODE_DEBUG
                puts("MAIN: reset");
#endif
                freeApp();
                delayUsIdle(1000000);
                data_initialized = 0;
                app_state = APP_INIT;
                break;
            case APP_EXIT:
#ifdef MODE_DEBUG
                puts("MAIN: exit");
#endif
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