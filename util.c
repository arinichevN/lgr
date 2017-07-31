/*
 * lgr
 */
#include "main.h"

FUN_LLIST_GET_BY_ID(Prog)

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

FUN_LOCK(Prog)
FUN_UNLOCK(Prog)

int tryLockProg(Prog *item) {
    if (item == NULL) {
        return 0;
    }
    if (pthread_mutex_trylock(&(item->mutex.self)) != 0) {
        return 0;
    }
    return 1;
}

struct timespec getTimeRestR(const Prog *item) {
    struct timespec out = {-1, -1};
    if (item->tmr.ready) {
        out = getTimeRest_ts(item->interval_min, item->tmr.start);
    }
    return out;
}

int bufCatProgInit(const Prog *item, char *buf, size_t buf_size) {
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR,
            item->id,
            item->interval_min.tv_sec,
            item->max_rows
            );

    if (bufCat(buf, q, buf_size) == NULL) {
        return 0;
    }
    return 1;
}

int bufCatProgRuntime(const Prog *item, char *buf, size_t buf_size) {
    char q[LINE_SIZE];
    char *state = getStateStr(item->state);
    struct timespec tm_rest = getTimeRestR(item);
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_ROW_STR,
            item->id,
            state,
            tm_rest.tv_sec
            );
    if (bufCat(buf, q, buf_size) == NULL) {
        return 0;
    }
    return 1;
}

int sendStrPack(char qnf, char *cmd) {
    extern Peer peer_client;
    return acp_sendStrPack(qnf, cmd, &peer_client);
}

int sendBufPack(char *buf, char qnf, char *cmd_str) {
    extern Peer peer_client;
    return acp_sendBufPack(buf, qnf, cmd_str, &peer_client);
}

void waitThread_ctl(char cmd) {
    thread_cmd = cmd;
    pthread_join(thread, NULL);
}

void sendStr(const char *s, uint8_t *crc) {
    acp_sendStr(s, crc, &peer_client);
}

void sendFooter(int8_t crc) {
    acp_sendFooter(crc, &peer_client);
}

void printAll() {
    char q[LINE_SIZE];
    uint8_t crc = 0;
    size_t i;
    snprintf(q, sizeof q, "CONFIG_FILE: %s\n", CONFIG_FILE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "pid_path: %s\n", pid_path);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "sock_buf_size: %d\n", sock_buf_size);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "cycle_duration sec: %ld\n", cycle_duration.tv_sec);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "cycle_duration nsec: %ld\n", cycle_duration.tv_nsec);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "db_data_path: %s\n", db_data_path);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "db_public_path: %s\n", db_public_path);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "db_log_path: %s\n", db_log_path);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "app_state: %s\n", getAppState(app_state));
    sendStr(q, &crc);
    snprintf(q, sizeof q, "PID: %d\n", proc_id);
    sendStr(q, &crc);

    sendStr("+-------------------------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                                Peer                                             |\n", &crc);
    sendStr("+--------------------------------+-----------+----------------+-----------+-----------+-----------+\n", &crc);
    sendStr("|               id               |  sin_port |      addr      |     fd    |  active   |   link    |\n", &crc);
    sendStr("+--------------------------------+-----------+----------------+-----------+-----------+-----------+\n", &crc);
    for (i = 0; i < peer_list.length; i++) {
        snprintf(q, sizeof q, "|%32s|%11u|%16u|%11d|%11d|%11p|\n",
                peer_list.item[i].id,
                peer_list.item[i].addr.sin_port,
                peer_list.item[i].addr.sin_addr.s_addr,
                *peer_list.item[i].fd,
                peer_list.item[i].active,
                (void *)&peer_list.item[i]
                );
        sendStr(q, &crc);
    }
    sendStr("+--------------------------------+-----------+----------------+-----------+-----------+-----------+\n", &crc);

    sendStr("+-----------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                      Program                                      |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("|     id    |   kind    | interval  | row_count |    link   | sensor_ptr| time_rest |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    PROG_LIST_LOOP_DF
    PROG_LIST_LOOP_ST
            struct timespec tm_rest = getTimeRest_ts(curr->interval_min, curr->tmr.start);
    snprintf(q, sizeof q, "|%11d|%11s|%11ld|%11d|%11p|%11p|%11ld|\n",
            curr->id,
            curr->kind,
            curr->interval_min.tv_sec,
            curr->max_rows,
            (void *) curr,
            (void *) &curr->sensor_fts,
            tm_rest.tv_sec
            );
    sendStr(q, &crc);
    PROG_LIST_LOOP_SP
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);

    sendFooter(crc);
}

void printHelp() {
    char q[LINE_SIZE];
    uint8_t crc = 0;
    sendStr("COMMAND LIST\n", &crc);
    snprintf(q, sizeof q, "%c\tput process into active mode; process will read configuration\n", ACP_CMD_APP_START);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tput process into standby mode; all running programs will be stopped\n", ACP_CMD_APP_STOP);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tfirst put process in standby and then in active mode\n", ACP_CMD_APP_RESET);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tterminate process\n", ACP_CMD_APP_EXIT);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget state of process; response: B - process is in active mode, I - process is in standby mode\n", ACP_CMD_APP_PING);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget some variable's values; response will be packed into multiple packets\n", ACP_CMD_APP_PRINT);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget this help; response will be packed into multiple packets\n", ACP_CMD_APP_HELP);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tload program into RAM and start its execution; program id expected if '.' quantifier is used\n", ACP_CMD_START);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tunload program from RAM; program id expected if '.' quantifier is used\n", ACP_CMD_STOP);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tunload program from RAM and then load it again; program id expected if '.' quantifier is used\n", ACP_CMD_RESET);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tenable running program; program id expected if '.' quantifier is used\n", ACP_CMD_LGR_PROG_ENABLE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tdisable running program; program id expected if '.' quantifier is used\n", ACP_CMD_LGR_PROG_DISABLE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget program info; response format: id_intervalSec_rowCount; program id expected if '.' quantifier is used\n", ACP_CMD_LGR_PROG_GET_DATA_INIT);
    sendStr(q, &crc);
    sendFooter(crc);
}
