
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
    //unique id
    for (int i = 0; i < list->length; i++) {
        for (int j = i + 1; j < list->length; j++) {
            if (list->item[i].id == list->item[j].id) {
                fprintf(stderr, "config_checkSensorFTSList: id = %d is not unique\n", list->item[i].id);
                return 0;
            }
        }
    }
    return 1;
}

int config_checkEMList(const EMList *list) {
    //unique id
    for (int i = 0; i < list->length; i++) {
        for (int j = i + 1; j < list->length; j++) {
            if (list->item[i].id == list->item[j].id) {
                fprintf(stderr, "config_checkEMList(): id = %d is not unique\n", list->item[i].id);
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
    int c = 0;
    for (int i = 0; i < argc; i++) {
        if (DB_COLUMN_IS("id")) {
            memcpy(PDLi.id, argv[i], NAME_SIZE);
            c++;
        } else if (DB_COLUMN_IS("port")) {
            PDLi.port = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("ip_addr")) {
            memcpy(PDLi.addr_str, argv[i], LINE_SIZE);
            c++;
        } else {
            fputs("getPeerList_callback(): unknown column\n", stderr);
        }
    }
#define N 3
    if (c != N) {
        peer_data->list->length++;
        fprintf(stderr, "getPeerList_callback(): required %d columns but %d found\n", N, c);
        return EXIT_FAILURE;
    }
#undef N
    if (!makeClientAddr(&PDLi.addr, PDLi.addr_str, PDLi.port)) {
        fprintf(stderr, "getPeerList_callback(): bad ip address for peer with id=%s\n", PDLi.id);
        peer_data->list->length++;
        return EXIT_FAILURE;
    }
    PDLi.addr_size = sizeof PDLi.addr;
    PDLi.fd = PDLd;
    PDLi.active = 0;
    peer_data->list->length++;
    return EXIT_SUCCESS;
}

#undef PDLd
#undef PDLi

static int getPhoneNumber_callback(void *data, int argc, char **argv, char **azColName) {
    S1List *item = data;
    int c = 0;
    for (int i = 0; i < argc; i++) {
        if (DB_COLUMN_IS("value")) {
            memcpy(&item->item[item->length * LINE_SIZE], argv[i], LINE_SIZE);
            c++;
        } else {
            fputs("getPhoneNumber_callback(): unknown column\n", stderr);
        }
    }
    item->length++;
#define N 1
    if (c != N) {
        fprintf(stderr, "getPhoneNumber_callback(): required %d columns but %d found\n", N, c);
        return EXIT_FAILURE;
    }
#undef N
    return EXIT_SUCCESS;
}

static int getSensorFTS_callback(void *data, int argc, char **argv, char **azColName) {
    SensorFTSData *item = data;
    int c = 0;
    for (int i = 0; i < argc; i++) {
        if (DB_COLUMN_IS("peer_id")) {
            Peer *peer = getPeerById(argv[i], item->peer_list);
            if (peer == NULL) {
                return EXIT_FAILURE;
            }
            item->sensor->peer = *peer;
            c++;
        } else if (DB_COLUMN_IS("remote_id")) {
            item->sensor->remote_id = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("sensor_id")) {
            item->sensor->id = atoi(argv[i]);
            c++;
        } else {
            fputs("getSensorFTS_callback(): unknown column\n", stderr);
        }
    }
#define N 3
    if (c != N) {
        fprintf(stderr, "getPeerList_callback(): required %d columns but %d found\n", N, c);
        return EXIT_FAILURE;
    }
#undef N
    return EXIT_SUCCESS;
}

static int getSensorFTSList_callback(void *data, int argc, char **argv, char **azColName) {
    SensorFTSListData *d = data;
    int c = 0;
    for (int i = 0; i < argc; i++) {
        if (DB_COLUMN_IS("peer_id")) {
            Peer *peer = getPeerById(argv[i], d->peer_list);
            if (peer == NULL) {
                return EXIT_FAILURE;
            }
            d->list->item[d->list->length].peer = *peer;
            c++;
        } else if (DB_COLUMN_IS("remote_id")) {
            d->list->item[d->list->length].remote_id = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("sensor_id")) {
            d->list->item[d->list->length].id = atoi(argv[i]);
            c++;
        } else {
            fputs("getSensorFTSList_callback(): unknown column\n", stderr);
        }
    }
    d->list->length++;
#define N 3
    if (c != N) {
        fprintf(stderr, "getSensorFTSList_callback(): required %d columns but %d found\n", N, c);
        return EXIT_FAILURE;
    }
#undef N
    return EXIT_SUCCESS;
}

static int getEM_callback(void *data, int argc, char **argv, char **azColName) {
    EMData *item = data;
    int c = 0;
    for (int i = 0; i < argc; i++) {
        if (DB_COLUMN_IS("peer_id")) {
            Peer *peer = getPeerById(argv[i], item->peer_list);
            if (peer == NULL) {
                return EXIT_FAILURE;
            }
            item->em->peer = *peer;
            c++;
        } else if (DB_COLUMN_IS("remote_id")) {
            item->em->remote_id = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("pwm_rsl")) {
            item->em->pwm_rsl = atof(argv[i]);
            c++;
        } else {
            fputs("getEM_callback(): unknown column\n", stderr);
        }
    }
    item->em->last_output = 0.0f;
#define N 3
    if (c != N) {
        fprintf(stderr, "getEM_callback(): required %d columns but %d found\n", N, c);
        return EXIT_FAILURE;
    }
#undef N
    return 0;
}

static int getEMList_callback(void *data, int argc, char **argv, char **azColName) {
    EMListData *d = data;
    int c = 0;
    for (int i = 0; i < argc; i++) {
        if (DB_COLUMN_IS("peer_id")) {
            Peer *peer = getPeerById(argv[i], d->peer_list);
            if (peer == NULL) {
                return EXIT_FAILURE;
            }
            d->list->item[d->list->length].peer = *peer;
            c++;
        } else if (DB_COLUMN_IS("remote_id")) {
            d->list->item[d->list->length].remote_id = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("em_id")) {
            d->list->item[d->list->length].id = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("pwm_rsl")) {
            d->list->item[d->list->length].pwm_rsl = atof(argv[i]);
            c++;
        } else {
            fputs("getEMList_callback(): unknown column\n", stderr);
        }
    }
    d->list->length++;
#define N 4
    if (c != N) {
        fprintf(stderr, "getEMList_callback(): required %d columns but %d found\n", N, c);
        return EXIT_FAILURE;
    }
#undef N
    return 0;
}

int config_getPeerList(PeerList *list, int *fd, const char *db_path) {
    RESET_LIST(list)
    sqlite3 *db;
    if (!db_openR(db_path, &db)) {
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
    if (!db_exec(db, q, getPeerList_callback, &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPeerList(): query failed\n");
#endif
        sqlite3_close(db);
        FREE_LIST(list);
        return 0;
    }
    sqlite3_close(db);
    if (!config_checkPeerList(list)) {
        FREE_LIST(list);
        return 0;
    }
    return 1;
}

int config_getSensorFTSList(SensorFTSList *list, PeerList *peer_list, const char *db_path) {
    RESET_LIST(list)
    sqlite3 *db;
    if (!db_openR(db_path, &db)) {
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
        fprintf(stderr, "config_getSensorFTSList(): failed to allocate memory\n");
        sqlite3_close(db);
        return 0;
    }
    SensorFTSListData data = {.list = list, .peer_list = peer_list};
    char *q = "select sensor_id, peer_id, remote_id FROM sensor_mapping";
    if (!db_exec(db, q, getSensorFTSList_callback, &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getSensorFTSList(): query failed\n");
#endif
        sqlite3_close(db);
        FREE_LIST(list);
        return 0;
    }
    sqlite3_close(db);
    if (!config_checkSensorFTSList(list)) {
        FREE_LIST(list);
        return 0;
    }
    return 1;
}

int config_getEMList(EMList *list, PeerList *peer_list, const char *db_path) {
    RESET_LIST(list)
    sqlite3 *db;
    if (!db_openR(db_path, &db)) {
        return 0;
    }
    int n = 0;
    char *qn = "select count(*) FROM em_mapping";
    db_getInt(&n, db, qn);
    if (n <= 0) {
        sqlite3_close(db);
        return 1;
    }
    if (!initEMList(list, n)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getEMList(): failed to allocate memory\n");
#endif
        sqlite3_close(db);
        return 0;
    }
    EMListData data = {.list = list, .peer_list = peer_list};
    char *q = "select em_id, peer_id, remote_id, pwm_rsl FROM em_mapping";
    if (!db_exec(db, q, getEMList_callback, &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getEMList(): query failed\n");
#endif
        sqlite3_close(db);
        FREE_LIST(list);
        return 0;
    }
    sqlite3_close(db);
    if (!config_checkEMList(list)) {
        FREE_LIST(list);
        return 0;
    }
    return 1;
}

int config_getSensorFTS(SensorFTS *item, int sensor_id, const PeerList *pl, sqlite3 *db) {
    char q[LINE_SIZE];
    SensorFTSData data = {item, pl};
    memset(item, 0, sizeof *item);
    snprintf(q, sizeof q, "select sensor_id, peer_id, remote_id from sensor_mapping where sensor_id=%d", sensor_id);
    if (!db_exec(db, q, getSensorFTS_callback, &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getSensorFTS: query failed\n");
#endif
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
    if (!db_exec(db, q, getEM_callback, &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getEM: query failed\n");
#endif
        return 0;
    }
    item->id = em_id;
    return 1;
}

int config_getPeer(Peer *item, const char * peer_id, int *fd, sqlite3 *dbl, const char *db_path) {
    if (dbl != NULL && db_path != NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPeer(): db xor db_path expected\n");
#endif
        return 0;
    }
    sqlite3 *db;
    if (db_path != NULL) {
        if (!db_openR(db_path, &db)) {
            return 0;
        }
    } else {
        db = dbl;
    }
    char q[LINE_SIZE];
    PeerList pl = {.item = item, .length = 0, .max_length = 1};
    PeerData data = {.list = &pl, .fd = fd};
    memset(item, 0, sizeof *item);
    snprintf(q, sizeof q, "SELECT id, port, ip_addr FROM peer where id='%s'", peer_id);
    if (!db_exec(db, q, getPeerList_callback, &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPeer(): query failed\n");
#endif
        if (db_path != NULL) {
            sqlite3_close(db);
        }
        return 0;
    }
    if (db_path != NULL) {
        sqlite3_close(db);
    }
    if (data.list->length != 1) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPeer: can't get peer: %s\n", peer_id);
#endif
        return 0;
    }

    return 1;
}

int config_getPhoneNumberListG(S1List *list, int group_id, const char *db_path) {
    RESET_LIST(list)
    sqlite3 *db;
    if (!db_openR(db_path, &db)) {
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
    list->item = malloc(list_size);
    if (list->item == NULL) {
#ifdef MODE_DEBUG
        fputs("config_getPhoneNumberList: failed to allocate memory\n", stderr);
#endif
        sqlite3_close(db);
        return 0;
    }
    list->max_length = n;
    memset(list->item, 0, list_size);
    snprintf(q, sizeof q, "select value from phone_number where group_id=%d", group_id);
    if (!db_exec(db, q, getPhoneNumber_callback, list)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPhoneNumberList: query failed\n");
#endif
        sqlite3_close(db);
        FREE_LIST(list);
        return 0;
    }
    if (list->length != n) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPhoneNumberList: bad length: %d < %d\n", list->length, n);
#endif
        sqlite3_close(db);
        FREE_LIST(list);
        return 0;
    }
    sqlite3_close(db);
    return 1;
}

int config_getPhoneNumberListO(S1List *list, const char *db_path) {
    RESET_LIST(list)
    sqlite3 *db;
    if (!db_openR(db_path, &db)) {
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
    list->item = malloc(list_size);
    if (list->item == NULL) {
#ifdef MODE_DEBUG
        fputs("config_getPhoneNumberListO: failed to allocate memory\n", stderr);
#endif
        sqlite3_close(db);
        return 0;
    }
    list->max_length = n;
    memset(list->item, 0, list_size);
    if (!db_exec(db, "select value from phone_number order by group_id", getPhoneNumber_callback, list)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPhoneNumberListO: select query failed\n");
#endif
        sqlite3_close(db);
        FREE_LIST(list);
        return 0;
    }
    if (list->length != n) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_getPhoneNumberListO: bad length: %d < %d\n", list->length, n);
#endif
        sqlite3_close(db);
        FREE_LIST(list);
        return 0;
    }
    sqlite3_close(db);
    return 1;
}

int config_saveProgLoad(int id, int v, sqlite3 *dbl, const char *db_path) {
    if (dbl != NULL && db_path != NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_saveProgLoad(): db xor db_path expected\n");
#endif
        return 0;
    }
    sqlite3 *db;
    if (db_path != NULL) {
        if (!db_open(db_path, &db)) {
            return 0;
        }
    } else {
        db = dbl;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update prog set load=%d where id=%d", v, id);
    if (!db_exec(db, q, 0, 0)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_saveProgLoad(): query failed\n");
#endif
        return 0;
    }
    return 1;
}

int config_saveProgEnable(int id, int v, sqlite3 *dbl, const char *db_path) {
    if (dbl != NULL && db_path != NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_saveProgEnable(): db xor db_path expected\n");
#endif
        return 0;
    }
    sqlite3 *db;
    if (db_path != NULL) {
        if (!db_open(db_path, &db)) {
            return 0;
        }
    } else {
        db = dbl;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update prog set enable=%d where id=%d", v, id);
    if (!db_exec(db, q, 0, 0)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "config_saveProgEnable(): query failed\n");
#endif
        return 0;
    }
    return 1;
}