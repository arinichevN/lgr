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
    size_t i;
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

    SEND_STR("+-------------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                                Peer                                             |\n")
    SEND_STR("+--------------------------------+-----------+----------------+-----------+-----------+-----------+\n")
    SEND_STR("|               id               |  sin_port |      addr      |     fd    |  active   |   link    |\n")
    SEND_STR("+--------------------------------+-----------+----------------+-----------+-----------+-----------+\n")
    for (i = 0; i < peer_list.length; i++) {
        snprintf(q, sizeof q, "|%32s|%11u|%16u|%11d|%11d|%11p|\n",
                peer_list.item[i].id,
                peer_list.item[i].addr.sin_port,
                peer_list.item[i].addr.sin_addr.s_addr,
                *peer_list.item[i].fd,
                peer_list.item[i].active,
                (void *)&peer_list.item[i]
                );
        SEND_STR(q)
    }
    SEND_STR("+--------------------------------+-----------+----------------+-----------+-----------+-----------+\n")

    SEND_STR("+-----------------------------------------------------------------------------------+\n")
    SEND_STR("|                                      Program                                      |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|     id    |   kind    | interval  | row_count |    link   | sensor_ptr| time_rest |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
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
