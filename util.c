#include "main.h"

FUN_LLIST_GET_BY_ID(Prog)
extern int getProgByIdFDB(int prog_id, Prog *item, PeerList *peer_list, sqlite3 *dbl, const char *db_path);

void stopProgThread(Prog *item) {
#ifdef MODE_DEBUG
    printf("signaling thread %d to cancel...\n", item->id);
#endif
    if (pthread_cancel(item->thread) != 0) {
#ifdef MODE_DEBUG
        perror("pthread_cancel()");
#endif
    }
    void * result;
#ifdef MODE_DEBUG
    printf("joining thread %d...\n", item->id);
#endif
    if (pthread_join(item->thread, &result) != 0) {
#ifdef MODE_DEBUG
        perror("pthread_join()");
#endif
    }
    if (result != PTHREAD_CANCELED) {
#ifdef MODE_DEBUG
        printf("thread %d not canceled\n", item->id);
#endif
    }
}

void stopAllProgThreads(ProgList * list) {
    PROG_LIST_LOOP_ST
#ifdef MODE_DEBUG
            printf("signaling thread %d to cancel...\n", item->id);
#endif
    if (pthread_cancel(item->thread) != 0) {
#ifdef MODE_DEBUG
        perror("pthread_cancel()");
#endif
    }
    PROG_LIST_LOOP_SP

    PROG_LIST_LOOP_ST
            void * result;
#ifdef MODE_DEBUG
    printf("joining thread %d...\n", item->id);
#endif
    if (pthread_join(item->thread, &result) != 0) {
#ifdef MODE_DEBUG
        perror("pthread_join()");
#endif
    }
    if (result != PTHREAD_CANCELED) {
#ifdef MODE_DEBUG
        printf("thread %d not canceled\n", item->id);
#endif
    }
    PROG_LIST_LOOP_SP
}

void freeProg(Prog * item) {
    freeSocketFd(&item->sock_fd);
    freeMutex(&item->mutex);
    free(item);
}

void freeProgList(ProgList * list) {
    Prog *item = list->top, *temp;
    while (item != NULL) {
        temp = item;
        item = item->next;
        freeProg(temp);
    }
    list->top = NULL;
    list->last = NULL;
    list->length = 0;
}
int checkProg(const Prog *item) {
    if (item->interval_min.tv_sec < 0 || item->interval_min.tv_nsec < 0) {
        fprintf(stderr, "checkProg(): negative interval_min where prog id = %d\n", item->id);
        return 0;
    }
    if (strcmp(item->kind, LOG_KIND_FTS) != 0) {
        fprintf(stderr, "checkProg(): bad kind where prog id = %d\n", item->id);
        return 0;
    }
    if (item->max_rows < 0) {
        fprintf(stderr, "checkProg(): negative max_rows where prog id = %d\n", item->id);
        return 0;
    }
    return 1;
}
char * getStateStr(char state) {
    switch (state) {
        case OFF:
            return "OFF";
            break;
        case INIT:
            return "INIT";
            break;
        case ACT:
            return "ACT";
            break;
        case DISABLE:
            return "DISABLE";
            break;
    }
    return "\0";
}

int lockProgList() {
    extern Mutex progl_mutex;
    if (pthread_mutex_lock(&(progl_mutex.self)) != 0) {
#ifdef MODE_DEBUG

        perror("ERROR: lockProgList: error locking mutex");
#endif 
        return 0;
    }
    return 1;
}

int tryLockProgList() {
    extern Mutex progl_mutex;
    if (pthread_mutex_trylock(&(progl_mutex.self)) != 0) {

        return 0;
    }
    return 1;
}

int unlockProgList() {
    extern Mutex progl_mutex;
    if (pthread_mutex_unlock(&(progl_mutex.self)) != 0) {
#ifdef MODE_DEBUG

        perror("ERROR: unlockProgList: error unlocking mutex");
#endif 
        return 0;
    }
    return 1;
}
int readFTS(SensorFTS *s) {
    return acp_readSensorFTS(s);
}

int saveFTS(Prog *item, const char *db_path) {
    if (item->max_rows <= 0) {
        return 0;
    }
    if (!file_exist(db_path)) {
#ifdef MODE_DEBUG
        fputs("saveFTS(): file not found\n", stderr);
#endif
        return 0;
    }
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
#ifdef MODE_DEBUG
        fputs("saveFTS(): db open failed\n", stderr);
#endif
        return 0;
    }
    int n = 0;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select count(*) from v_real where id=%d", item->id);
    if (!db_getInt(&n, db, q)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "saveFTS(): failed to count from v_real\n");
#endif
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
            fprintf(stderr, "saveFTS(): insert failed\n");
#endif
        }
    } else {
        snprintf(q, sizeof q, "update v_real set mark = %ld, value = %f, status = '%s' where id = %d and mark = (select min(mark) from v_real where id = %d)", now.tv_sec, item->sensor_fts.value.value, status, item->id, item->id);
        if (!db_exec(db, q, 0, 0)) {
#ifdef MODE_DEBUG
            fprintf(stderr, "saveFTS(): update failed\n");
#endif
            return 0;
        }
    }
    sqlite3_close(db);
    return 0;
}

struct timespec getTimeRestR(const Prog *item) {
    struct timespec out = {-1, -1};
    if (item->tmr.ready) {
        out = getTimeRest_ts(item->interval_min, item->tmr.start);
    }
    return out;
}

int bufCatProgInit(const Prog *item, ACPResponse *response) {
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR,
            item->id,
            item->interval_min.tv_sec,
            item->max_rows
            );

    return acp_responseStrCat(response, q);
}

int bufCatProgRuntime(const Prog *item, ACPResponse *response) {
    char q[LINE_SIZE];
    char *state = getStateStr(item->state);
    struct timespec tm_rest = getTimeRestR(item);
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_ROW_STR,
            item->id,
            state,
            tm_rest.tv_sec
            );
    return acp_responseStrCat(response, q);
}



void printData(ACPResponse *response) {
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "CONFIG_FILE: %s\n", CONFIG_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    SEND_STR(q)
    snprintf(q, sizeof q, "cycle_duration sec: %ld\n", cycle_duration.tv_sec);
    SEND_STR(q)
    snprintf(q, sizeof q, "cycle_duration nsec: %ld\n", cycle_duration.tv_nsec);
    SEND_STR(q)
    snprintf(q, sizeof q, "db_data_path: %s\n", db_data_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "db_public_path: %s\n", db_public_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "db_log_path: %s\n", db_log_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "app_state: %s\n", getAppState(app_state));
    SEND_STR(q)
    snprintf(q, sizeof q, "PID: %d\n", getpid());
    SEND_STR(q)

    acp_sendPeerListInfo(&peer_list, response, &peer_client);

    SEND_STR("+-----------------------------------------------------------------------------------+\n")
    SEND_STR("|                                      Program                                      |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|     id    |   kind    | interval  | row_count | remote_id |  peer_id  | time_rest |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    
    PROG_LIST_LOOP_ST
            struct timespec tm_rest = getTimeRest_ts(item->interval_min, item->tmr.start);
    snprintf(q, sizeof q, "|%11d|%11s|%11ld|%11d|%11d|%11s|%11ld|\n",
            item->id,
            item->kind,
            item->interval_min.tv_sec,
            item->max_rows,
            item->sensor_fts.remote_id,
            item->sensor_fts.peer.id,
            tm_rest.tv_sec
            );
    SEND_STR(q)
    PROG_LIST_LOOP_SP
    SEND_STR_L("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")

}

void printHelp(ACPResponse *response) {
    char q[LINE_SIZE];
    SEND_STR("COMMAND LIST\n")
    snprintf(q, sizeof q, "%s\tput process into active mode; process will read configuration\n", ACP_CMD_APP_START);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tput process into standby mode; all running programs will be stopped\n", ACP_CMD_APP_STOP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tfirst put process in standby and then in active mode\n", ACP_CMD_APP_RESET);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tterminate process\n", ACP_CMD_APP_EXIT);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget state of process; response: B - process is in active mode, I - process is in standby mode\n", ACP_CMD_APP_PING);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget some variable's values; response will be packed into multiple packets\n", ACP_CMD_APP_PRINT);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget this help; response will be packed into multiple packets\n", ACP_CMD_APP_HELP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tload program into RAM and start its execution; program id expected\n", ACP_CMD_PROG_START);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tunload program from RAM; program id expected\n", ACP_CMD_PROG_STOP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tunload program from RAM and then load it again; program id expected\n", ACP_CMD_PROG_RESET);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tenable running program; program id expected\n", ACP_CMD_PROG_ENABLE);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tdisable running program; program id expected\n", ACP_CMD_PROG_DISABLE);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget program info; response format: id_intervalSec_rowCount; program id expected\n", ACP_CMD_PROG_GET_DATA_INIT);
    SEND_STR_L(q)
}
