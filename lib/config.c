#include "config.h"

int config_getStrValFromTbl(PGconn *db_conn, const char *id, char *value, const char *tbl, size_t value_size) {
    PGresult *r;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select value from public.%s where id='%s'", tbl, id);
    if ((r = dbGetDataT(db_conn, q, q)) == NULL) {
        return 0;
    }
    if (PQntuples(r) == 1) {
        memcpy(value, PQgetvalue(r, 0, 0), value_size);
        PQclear(r);
        return 1;
    }
#ifdef MODE_DEBUG
    fputs("config_getPidPath: one tuple expected\n", stderr);
#endif
    PQclear(r);
    return 0;
}

int config_getBufSize(PGconn *db_conn, const char *id, size_t *value) {
    PGresult *r;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select value from public.buf_size where id='%s'", id);
    if ((r = dbGetDataT(db_conn, q, q)) == NULL) {
        return 0;
    }
    if (PQntuples(r) == 1) {
        *value = atoi(PQgetvalue(r, 0, 0));
        PQclear(r);
        return 1;
    }
#ifdef MODE_DEBUG
    fputs("config_getBufSize: one tuple expected\n", stderr);
#endif
    PQclear(r);
    return 0;
}

int config_getCycleDurationUs(PGconn *db_conn, const char *id, struct timespec *value) {
    PGresult *r;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select value from public.cycle_duration_us where id='%s'", id);
    if ((r = dbGetDataT(db_conn, q, q)) == NULL) {
        return 0;
    }
    if (PQntuples(r) == 1) {
        *value = usToTimespec(atoi(PQgetvalue(r, 0, 0)));
        PQclear(r);
        return 1;
    }
#ifdef MODE_DEBUG
    fputs("config_getCycleDurationUs: one tuple expected\n", stderr);
#endif
    PQclear(r);
    return 0;
}

/*
int config_getUDPPort(PGconn *db_conn, const char *id, size_t *value) {
    PGresult *r;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select udp_port from public.peer where id='%s'", id);
    if ((r = dbGetDataT(db_conn, q, q)) == NULL) {
        return 0;
    }
    if (PQntuples(r) == 1) {
        *value = atoi(PQgetvalue(r, 0, 0));
        PQclear(r);
        return 1;
    }
#ifdef MODE_DEBUG
    fputs("config_getPort: one tuple expected\n", stderr);
#endif
    PQclear(r);
    return 0;
}
*/

int config_getPidPath(PGconn *db_conn, const char *id, char *value, size_t value_size) {
    PGresult *r;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select value from public.pid_path where id='%s'", id);
    if ((r = dbGetDataT(db_conn, q, q)) == NULL) {
        return 0;
    }
    if (PQntuples(r) == 1) {
        memcpy(value, PQgetvalue(r, 0, 0), value_size);
        PQclear(r);
        return 1;
    }
#ifdef MODE_DEBUG
    fputs("config_getPidPath: one tuple expected\n", stderr);
#endif
    PQclear(r);
    return 0;
}

int config_getI2cPath(PGconn *db_conn, const char *id, char *value, size_t value_size) {
    PGresult *r;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select value from public.i2c_path where id='%s'", id);
    if ((r = dbGetDataT(db_conn, q, q)) == NULL) {
        return 0;
    }
    if (PQntuples(r) == 1) {
        memcpy(value, PQgetvalue(r, 0, 0), value_size);
        PQclear(r);
        return 1;
    }
#ifdef MODE_DEBUG
    fputs("config_getI2cPath: one tuple expected\n", stderr);
#endif
    PQclear(r);
    return 0;
}

int config_getDbConninfo(PGconn *db_conn, const char *id, char *value, size_t value_size) {
    PGresult *r;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select value from public.db_conninfo where id='%s'", id);
    if ((r = dbGetDataT(db_conn, q, q)) == NULL) {
        return 0;
    }
    if (PQntuples(r) == 1) {
        memcpy(value, PQgetvalue(r, 0, 0), value_size);
        PQclear(r);
        return 1;
    }
#ifdef MODE_DEBUG
    fputs("config_getPidPath: one tuple expected\n", stderr);
#endif
    PQclear(r);
    return 0;
}

int config_getLockKey(PGconn *db_conn, const char *id, char *value, size_t value_size) {
    PGresult *r;
    char q[LINE_SIZE];
    size_t i;
    for (i = 0; i < value_size; i++) {
        value[i] = '\0';
    }
    snprintf(q, sizeof q, "select value from public.lock_key where id='%s'", id);
    if ((r = dbGetDataT(db_conn, q, q)) == NULL) {
        return 0;
    }
    if (PQntuples(r) == 1) {
        memcpy(value, PQgetvalue(r, 0, 0), value_size);
        PQclear(r);
        return 1;
    }
#ifdef MODE_DEBUG
    fputs("config_getLockKey: one tuple expected\n", stderr);
#endif
    PQclear(r);
    return 0;
}

int config_checkPeerList(const PeerList *list) {
    size_t i, j;
    //unique id
    for (i = 0; i < list->length; i++) {
        for (j = i + 1; j < list->length; j++) {
            if (strcmp(list->item[i].id, list->item[j].id) == 0) {
                fprintf(stderr, "checkPeerList: id = %s is not unique\n", list->item[i].id);
                return 0;
            }
        }
    }
    return 1;
}

int config_getPeerList(PGconn *db_conn, PeerList *list, int *fd) {
    PGresult *r;
    size_t i;
    list->length = 0;
    list->item = NULL;
    char *q = "select id, udp_port, ip_addr from public.peer";
    if ((r = dbGetDataT(db_conn, q, q)) == NULL) {
        return 0;
    }
    list->length = PQntuples(r);
    if (list->length > 0) {
        list->item = (Peer *) malloc(list->length * sizeof *(list->item));
        if (list->item == NULL) {
            list->length = 0;
            fputs("config_getPeerList: ERROR: failed to allocate memory\n", stderr);
            PQclear(r);
            return 0;
        }
        for (i = 0; i < list->length; i++) {
            memcpy(list->item[i].id, PQgetvalue(r, i, 0), NAME_SIZE);
            int port = atoi(PQgetvalue(r, i, 1));
            char addr_str[NAME_SIZE];
            memset(addr_str, 0, sizeof addr_str);
            memcpy(addr_str, PQgetvalue(r, i, 2), NAME_SIZE);
            if (!makeClientAddr(&list->item[i].addr, addr_str, port)) {
                PQclear(r);
                fprintf(stderr, "config_getPeerList: ERROR: bad ip address for peer with id=%s\n", list->item[i].id);
                return 0;
            }
            list->item[i].addr_size = sizeof list->item[i].addr;
            list->item[i].fd = fd;
            if (!initMutex(&list->item[i].mutex)) {
                PQclear(r);
                fprintf(stderr, "config_getPeerList: ERROR: initMutex() failed for peer with id=%s\n", list->item[i].id);
                return 0;
            }
        }
    }
    PQclear(r);
    if (!config_checkPeerList(list)) {
        return 0;
    }
    return 1;
}