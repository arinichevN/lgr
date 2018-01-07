#include "dbl.h"
int db_open(const char *path, sqlite3 **db) {
    char q[LINE_SIZE * 2];
    snprintf(q, sizeof q, "file://%s", path);
    int rc = sqlite3_open_v2(path, db, SQLITE_OPEN_READWRITE, NULL);
    if (rc != SQLITE_OK) {
#ifdef MODE_DEBUG
        fprintf(stderr, "db_open: path: %s, status: %d, message: %s\n", path, rc, sqlite3_errmsg(*db));
#endif
        sqlite3_close(*db);
        return 0;
    }
    return 1;
}

int db_exec(sqlite3 *db, char *q, int (*callback)(void*, int, char**, char**), void * data) {
    char *errMsg = 0;
    int rc = sqlite3_exec(db, q, callback, data, &errMsg);
    if (rc != SQLITE_OK) {
#ifdef MODE_DEBUG
        fprintf(stderr, "db_exec: status: %d, %s\n", rc, errMsg);
#endif     
        sqlite3_free(errMsg);

        return 0;
    }
    return 1;
}

static int getInt_callback(void *data, int argc, char **argv, char **azColName) {
    int * item = (int *) data;
    if (argc >= 1) {
        *item = atoi(argv[0]);
    }
    return 0;
}

int db_getInt(int *item, sqlite3 *db, char *q) {
    void *data = item;
    if (!db_exec(db, q, getInt_callback, data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "db_getInt: failed\n");
#endif
        return 0;
    }
    return 1;
}
