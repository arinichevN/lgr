#include "dbl.h"
int db_open(const char *path, sqlite3 **db) {
    char q[LINE_SIZE * 2];
    snprintf(q, sizeof q, "file://%s", path);
    int rc = sqlite3_open_v2(path, db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, NULL);
    if (rc != SQLITE_OK) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): path: %s, status: %d, message: %s\n",__FUNCTION__, path, rc, sqlite3_errmsg(*db));
#endif
        sqlite3_close(*db);
        return 0;
    }
    return 1;
}

int db_openR(const char *path, sqlite3 **db) {
    char q[LINE_SIZE * 2];
    snprintf(q, sizeof q, "file://%s", path);
    int rc = sqlite3_open_v2(path, db, SQLITE_OPEN_READONLY |  SQLITE_OPEN_FULLMUTEX, NULL);
    if (rc != SQLITE_OK) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): path: %s, status: %d, message: %s\n",__FUNCTION__, path, rc, sqlite3_errmsg(*db));
#endif
        sqlite3_close(*db);
        return 0;
    }
    //int rs= sqlite3_busy_timeout(*db, 2000);
    return 1;
}

int db_exec(sqlite3 *db, char *q, int (*callback)(void*, int, char**, char**), void * data) {
    char *errMsg = 0;
   
    int rc = sqlite3_exec(db, q, callback, data, &errMsg);
    if (rc != SQLITE_OK) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): status: %d, query: %s, message: %s\n",__FUNCTION__, rc, q, errMsg);
#endif     
        sqlite3_free(errMsg);

        return 0;
    }
    return 1;
}

static int getInt_callback(void *data, int argc, char **argv, char **azColName) {
    int * item = data;
    if (argc >= 1) {
        *item = atoi(argv[0]);
    }
    return 0;
}

int db_getInt(int *item, sqlite3 *db, char *q) {
    void *data = item;
    if (!db_exec(db, q, getInt_callback, data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "-%s(): failed\n",__FUNCTION__);
#endif
        return 0;
    }
    return 1;
}


int db_saveTableFieldFloat(const char * table, const char *field, int id, float value, sqlite3 *dbl, const char* db_path) {
    if (dbl != NULL && db_path != NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): dbl xor db_path expected\n", __FUNCTION__);
#endif
        return 0;
    }
    sqlite3 *db;
    int close=0;
    if (db_path != NULL) {
        if (!db_open(db_path, &db)) {
#ifdef MODE_DEBUG
            fprintf(stderr, "%s(): failed to open database\n", __FUNCTION__);
#endif
            return 0;
        }
        close=1;
    } else {
        db = dbl;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update '%s' set '%s'=%f where id=%d", table, field, value, id);
    if (!db_exec(db, q, 0, 0)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): query failed\n", __FUNCTION__);
#endif
        if (close) sqlite3_close(db);
        return 0;
    }
    if (close) sqlite3_close(db);
    return 1;
}

int db_saveTableFieldInt(const char * table, const char *field, int id, int value, sqlite3 *dbl, const char* db_path) {
    if (dbl != NULL && db_path != NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): dbl xor db_path expected\n", __FUNCTION__);
#endif
        return 0;
    }
    sqlite3 *db;int close=0;
    if (db_path != NULL) {
        if (!db_open(db_path, &db)) {
#ifdef MODE_DEBUG
            fprintf(stderr, "%s(): failed to open database\n", __FUNCTION__);
#endif
            return 0;
        }
        close=1;
    } else {
        db = dbl;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update '%s' set '%s'=%d where id=%d", table, field, value, id);
    if (!db_exec(db, q, 0, 0)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): query failed\n", __FUNCTION__);
#endif
        if (close) sqlite3_close(db);
        return 0;
    }
    if (close) sqlite3_close(db);
    return 1;
}

int db_saveTableFieldText(const char * table, const char *field, int id, const char * value, sqlite3 *dbl, const char* db_path) {
    if (dbl != NULL && db_path != NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): dbl xor db_path expected\n", __FUNCTION__);
#endif
        return 0;
    }
    sqlite3 *db;
    int close = 0;
    if (db_path != NULL) {
        if (!db_open(db_path, &db)) {
#ifdef MODE_DEBUG
            fprintf(stderr, "%s(): failed to open database\n", __FUNCTION__);
#endif
            return 0;
        }
        close = 1;
    } else {
        db = dbl;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update '%s' set '%s'='%s' where id=%d", table, field, value, id);
    if (!db_exec(db, q, 0, 0)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): query failed\n", __FUNCTION__);
#endif
        if (close) sqlite3_close(db);
        return 0;
    }
    if (close) sqlite3_close(db);
    return 1;
}
