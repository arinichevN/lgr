#include "logl.h"
int log_saveAlert(char * message, unsigned int limit, const char *db_path) {
    if (limit <= 0) {
        return 0;
    }
    if (!file_exist(db_path)) {
#ifdef MODE_DEBUG
        fputs("log_saveAlert: file not found\n", stderr);
#endif
        return 0;
    }
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
#ifdef MODE_DEBUG
        fputs("log_saveAlert: db open failed\n", stderr);
#endif
        return 0;
    }
    int n = 0;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select count(*) from alert");
    if (!db_getInt(&n, db, q)) {
#ifdef MODE_DEBUG
        fputs("log_saveAlert: getCount failed\n", stderr);
#endif
        sqlite3_close(db);
        return 0;
    }
    struct timespec tm1; tm1= getCurrentTime();
    if (n < limit) {
        snprintf(q, sizeof q, "insert into alert(mark, message) values (%ld, '%s')", tm1.tv_sec, message);
        if (!db_exec(db, q, 0, 0)) {
#ifdef MODE_DEBUG
            fputs("log_saveAlert: insert failed\n", stderr);
#endif
        }
    } else {
        snprintf(q, sizeof q, "update alert set mark = %ld, message = '%s' where mark = (select min(mark) from alert)", tm1.tv_sec, message);
        if (!db_exec(db, q, 0, 0)) {
#ifdef MODE_DEBUG
            fputs("log_saveAlert: update failed\n", stderr);
#endif
            return 0;
        }
    }
    sqlite3_close(db);
    return 0;
}