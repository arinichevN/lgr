
#include "acp/main.h"
#include "configl.h"

int config_checkPeerList(const PeerList *list) {
    //unique id
    for (size_t i = 0; i < list->length; i++) {
        for (size_t j = i + 1; j < list->length; j++) {
            if (strcmp(list->item[i].id, list->item[j].id) == 0) {
                fprintf(stderr, "checkPeerList: id = %s is not unique\n", list->item[i].id);
                return 0;
            }
        }
    }
    return 1;
}

int config_checkSensorFTSList(const SensorFTSList *list) {
    for (size_t i = 0; i < list->length; i++) {
        if (list->item[i].source == NULL) {
            fprintf(stderr, "config_checkSensorFTSList: bad peer where id = %d\n", list->item[i].id);
            return 0;
        }
    }
    //unique id
    for (size_t i = 0; i < list->length; i++) {
        for (size_t j = i + 1; j < list->length; j++) {
            if (list->item[i].id == list->item[j].id) {
                fprintf(stderr, "config_checkSensorFTSList: id = %d is not unique\n", list->item[i].id);
                return 0;
            }
        }
    }
    return 1;
}
#define PDLd peer_data->fd
#define PDLi peer_data->list->item[peer_data->list->length]

static int getPeerList_callback(void *data, int argc, char **argv, char **azColName) {
    PeerData *peer_data = data;
    int i;
    for (i = 0; i < argc; i++) {
        if (strcmp("id", azColName[i]) == 0) {
            memcpy(PDLi.id, argv[i], NAME_SIZE);
        } else if (strcmp("port", azColName[i]) == 0) {
            PDLi.port = atoi(argv[i]);
        } else if (strcmp("ip_addr", azColName[i]) == 0) {
            memcpy(PDLi.addr_str, argv[i], LINE_SIZE);
        } else {
            fputs("getPeerList_callback: unknown column\n", stderr);
        }
    }
    if (!makeClientAddr(&PDLi.addr, PDLi.addr_str, PDLi.port)) {
        fprintf(stderr, "getPeerList_callback: ERROR: bad ip address for peer with id=%s\n", PDLi.id);
        peer_data->list->length++;
        return 1;
    }
    PDLi.addr_size = sizeof PDLi.addr;
    PDLi.fd = PDLd;
    PDLi.active = 0;
    if (!initMutex(&PDLi.mutex)) {
        fprintf(stderr, "getPeerList_callback: ERROR: initMutex() failed for peer with id=%s\n", PDLi.id);
        peer_data->list->length++;
        return 1;
    }
    peer_data->list->length++;
    return 0;
}

static int getPhoneNumber_callback(void *data, int argc, char **argv, char **azColName) {
    S1List *item = (S1List *) data;
    int i;
    for (i = 0; i < argc; i++) {
        if (strcmp("value", azColName[i]) == 0) {
            memcpy(&item->item[item->length * LINE_SIZE], argv[i], LINE_SIZE);
            puts(argv[i]);
        } else {
            fputs("getPhoneNumber_callback: unknown column\n", stderr);
        }
    }
    item->length++;
    return 0;
}

static int getSensorFTS_callback(void *data, int argc, char **argv, char **azColName) {
    SensorFTSData *item = data;
    int i;
    for (i = 0; i < argc; i++) {
        if (strcmp("peer_id", azColName[i]) == 0) {
            item->sensor->source = getPeerById(argv[i], item->peer_list);
        } else if (strcmp("remote_id", azColName[i]) == 0) {
            item->sensor->remote_id = atoi(argv[i]);
        } else if (strcmp("sensor_id", azColName[i]) == 0) {
            item->sensor->id = atoi(argv[i]);
        } else {
            fputs("getSensorFTS_callback: unknown column\n", stderr);
        }
    }
    return 0;
}

static int getSensorFTSList_callback(void *data, int argc, char **argv, char **azColName) {
    SensorFTSListData *d = (SensorFTSListData *) data;
    int i;
    for (i = 0; i < argc; i++) {
        if (strcmp("peer_id", azColName[i]) == 0) {
            d->list->item[d->list->length].source = getPeerById(argv[i], d->peer_list);
        } else if (strcmp("remote_id", azColName[i]) == 0) {
            d->list->item[d->list->length].remote_id = atoi(argv[i]);
        } else if (strcmp("sensor_id", azColName[i]) == 0) {
            d->list->item[d->list->length].id = atoi(argv[i]);
        } else {
            fputs("getSensorFTSList_callback: unknown column\n", stderr);
        }
    }
    d->list->length++;
    return 0;
}

static int getEM_callback(void *data, int argc, char **argv, char **azColName) {
    EMData *item = data;
    int i;
    for (i = 0; i < argc; i++) {
        if (strcmp("peer_id", azColName[i]) == 0) {
            item->em->source = getPeerById(argv[i], item->peer_list);
        } else if (strcmp("remote_id", azColName[i]) == 0) {
            item->em->remote_id = atoi(argv[i]);
        } else if (strcmp("pwm_rsl", azColName[i]) == 0) {
            item->em->pwm_rsl = atof(argv[i]);
        } else {
            fputs("getEM_callback: unknown column\n", stderr);
        }
    }
    item->em->last_output = 0.0f;
    return 0;
}

int config_getPeerList(PeerList *list, int *fd, const char *db_path) {
    list->item = NULL;
    list->length = list->max_length = 0;
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    int n = 0;
    char *qn = "select count(*) FROM peer";
    db_getInt(&n, db, qn);
    if (n <= 0) {
        sqlite3_close(db);
        return 1;
    }
    if (!initPeerList(list, n)) {
        fprintf(stderr, "config_getPeerList: failed to allocate memory\n");
        sqlite3_close(db);
        return 0;
    }
    PeerData data = {.list = list, .fd = fd};
    char *q = "select id, port, ip_addr FROM peer";
    if (!db_exec(db, q, getPeerList_callback, (void*) &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPeerList: query failed: %s\n", q);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    if (!config_checkPeerList(list)) {
        return 0;
    }
    return 1;
}

int config_getSensorFTSList(SensorFTSList *list, PeerList *peer_list, const char *db_path) {
    list->item = NULL;
    list->length = list->max_length = 0;
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    int n = 0;
    char *qn = "select count(*) FROM sensor_mapping";
    db_getInt(&n, db, qn);
    if (n <= 0) {
        sqlite3_close(db);
        return 1;
    }
    if (!initSensorFTSList(list, n)) {
        fprintf(stderr, "config_getSensorFTSList: failed to allocate memory\n");
        sqlite3_close(db);
        return 0;
    }
    SensorFTSListData data = {.list = list, .peer_list = peer_list};
    char *q = "select sensor_id, peer_id, peer_id FROM sensor_mapping";
    if (!db_exec(db, q, getSensorFTSList_callback, (void*) &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getSensorFTSList: query failed: %s\n", q);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    if (!config_checkSensorFTSList(list)) {
        return 0;
    }
    return 1;
}

int config_getSensorFTS(SensorFTS *item, int sensor_id, const PeerList *pl, sqlite3 *db) {
    char q[LINE_SIZE];
    SensorFTSData data = {item, pl};
    memset(item, 0, sizeof *item);
    snprintf(q, sizeof q, "select sensor_id, peer_id, remote_id from sensor_mapping where sensor_id=%d", sensor_id);
    if (!db_exec(db, q, getSensorFTS_callback, (void*) &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getSensorFTS: query failed: %s\n", q);
#endif
        return 0;
    }
    if (!initMutex(&item->mutex)) {
        return 0;
    }
    item->id = sensor_id;
    return 1;
}

int config_getEM(EM *item, int em_id, const PeerList *pl, sqlite3 *db) {
    char q[LINE_SIZE];
    EMData data = {item, pl};
    memset(item, 0, sizeof *item);
    snprintf(q, sizeof q, "select peer_id, remote_id, pwm_rsl from em_mapping where em_id=%d", em_id);
    if (!db_exec(db, q, getEM_callback, (void*) &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getEM: query failed: %s\n", q);
#endif
        return 0;
    }
    if (!initMutex(&item->mutex)) {
        return 0;
    }
    item->id = em_id;
    return 1;
}

int config_getPeer(Peer *item, char * peer_id, int *fd, sqlite3 *db) {
    char q[LINE_SIZE];
    PeerList pl = {.item = item, .length = 0};
    PeerData data = {.list = &pl, .fd = fd};
    memset(item, 0, sizeof *item);
    snprintf(q, sizeof q, "SELECT id, port, ip_addr FROM peer where id='%s'", peer_id);
    if (!db_exec(db, q, getPeerList_callback, (void*) &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPeer: query failed: %s\n", q);
#endif
        return 0;
    }
    if (data.list->length != 1) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPeer: can't get peer: %s\n", peer_id);
#endif
        return 0;
    }
    if (!initMutex(&item->mutex)) {
        return 0;
    }
    return 1;
}

int config_getPhoneNumberListG(S1List *list, int group_id, const char *db_path) {
    list->item = NULL;
    list->length = list->max_length = 0;
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    char q[LINE_SIZE];
    int n = 0;
    snprintf(q, sizeof q, "select count(*) from phone_number where group_id=%d", group_id);
    if (!db_getInt(&n, db, q)) {
#ifdef MODE_DEBUG
        fputs("config_getPhoneNumberList: getCount failed", stderr);
#endif
        sqlite3_close(db);
        return 0;
    }
    if (n <= 0) {
        sqlite3_close(db);
        return 1;
    }
    size_t list_size = LINE_SIZE * n * sizeof *(list->item);
    list->item = (char *) malloc(list_size);
    if (list->item == NULL) {
#ifdef MODE_DEBUG
        fputs("config_getPhoneNumberList: failed to allocate memory\n", stderr);
#endif
        sqlite3_close(db);
        return 0;
    }
    memset(list->item, 0, list_size);
    snprintf(q, sizeof q, "select value from phone_number where group_id=%d", group_id);
    if (!db_exec(db, q, getPhoneNumber_callback, (void*) list)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPhoneNumberList: query failed: %s\n", q);
#endif
        free(list->item);
        sqlite3_close(db);
        return 0;
    }
    if (list->length != n) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPhoneNumberList: bad length: %d < %d\n", list->length, n);
#endif
        free(list->item);
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    return 1;
}

int config_getPhoneNumberListO(S1List *list, const char *db_path) {
    list->item = NULL;
    list->length = list->max_length = 0;
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    int n = 0;
    if (!db_getInt(&n, db, "select count(*) from phone_number")) {
#ifdef MODE_DEBUG
        fputs("config_getPhoneNumberListO: getCount failed", stderr);
#endif
        sqlite3_close(db);
        return 0;
    }
    if (n <= 0) {
        sqlite3_close(db);
        return 1;
    }
    size_t list_size = LINE_SIZE * n * sizeof *(list->item);
    list->item = (char *) malloc(list_size);
    if (list->item == NULL) {
#ifdef MODE_DEBUG
        fputs("config_getPhoneNumberListO: failed to allocate memory\n", stderr);
#endif
        sqlite3_close(db);
        return 0;
    }
    memset(list->item, 0, list_size);
    if (!db_exec(db, "select value from phone_number order by group_id", getPhoneNumber_callback, (void*) list)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPhoneNumberListO: select query failed\n");
#endif
        free(list->item);
        sqlite3_close(db);
        return 0;
    }
    if (list->length != n) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPhoneNumberListO: bad length: %d < %d\n", list->length, n);
#endif
        free(list->item);
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    return 1;
}