
#include "dbp.h"

int dbp_init(PGconn **conn, char *conninfo) {
    if (PQstatus(*conn) == CONNECTION_OK) {
        return 1;
    }
    *conn = PQconnectdb(conninfo);
    if (PQstatus(*conn) != CONNECTION_OK) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): %s\n", F, PQerrorMessage(*conn));
#endif
        PQfinish(*conn);
        return 0;
    }
    return 1;
}

void dbp_free(PGconn *conn) {
    if (PQstatus(conn) == CONNECTION_OK) {
        PQfinish(conn);
    }
}

/*
 * for command returning NO data
*/

int dbp_cmd(PGconn *conn, char *q) {
    int out = 0;
    PGresult *r = PQexec(conn, q);
    if (r == NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): %s: no result: %s\n", F, q, PQerrorMessage(conn));
#endif
        return out;
    }
    if (PQresultStatus(r) == PGRES_COMMAND_OK) {
        out = 1;
    }
#ifdef MODE_DEBUG
    if (out == 0) {
        fprintf(stderr, "%s(): %s: command is not ok\n", F, q);
    }
#endif
    PQclear(r);
    return out;
}

/*
 *  for command returning data
*/

PGresult *dbp_exec(PGconn *conn, char *q) {
    PGresult *r = PQexec(conn, q);
    if (r == NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): %s: no result: %s\n", F, q, PQerrorMessage(conn));
#endif
        return r;
    }
    if (PQresultStatus(r) != PGRES_TUPLES_OK) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): %s: tuples are not ok\n", F, q);
#endif
        PQclear(r);
        r = NULL;
    }
    return r;
}
//for use with select count(*)

int dbp_getInt(int *item, PGconn *conn, char *q) {
    PGresult *r = PQexec(conn, q);
    if (r == NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): %s: no result: %s\n", F, q, PQerrorMessage(conn));
#endif
        return 0;
    }
    if (PQresultStatus(r) != PGRES_TUPLES_OK) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): %s: tuples are not ok\n", F, q);
#endif
        PQclear(r);
        return 0;
    }
   int n = PQntuples(r);
    if (n != 1) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): %s: tuples: need only one, but %d given\n", F, q, n);
#endif
        PQclear(r);
        return 0;
    }
    *item = atoi(PQgetvalue(r, 0, 0));
    PQclear(r);
    return 1;
}

int dbp_conninfoParse(const char *buf, char *host, int *port, char *dbname, char *user, size_t str_size) {
    int i, host_found = 0, port_found = 0, dbname_found = 0, user_found = 0;
    *port = 0;
    memset(host, 0, str_size);
    memset(dbname, 0, str_size);
    memset(user, 0, str_size);
    for (i = 0; i < strlen(buf); i++) {
        switch (buf[i]) {
            case 'h':
                if (host_found) {
                    break;
                }
                if (sscanf(buf + i, "host=%s ", host) == 1) {
                    host_found = 1;
                }
                break;
            case 'p':
                if (port_found) {
                    break;
                }
                if (sscanf(buf + i, "port=%d ", port) == 1) {
                    port_found = 1;
                }
                break;
            case 'd':
                if (dbname_found) {
                    break;
                }
                if (sscanf(buf + i, "dbname=%s ", dbname) == 1) {
                    dbname_found = 1;
                }
                break;
            case 'u':
                if (dbname_found) {
                    break;
                }
                if (sscanf(buf + i, "user=%s ", user) == 1) {
                    user_found = 1;
                }
                break;
        }
    }
    return 1;
}

int dbp_conninfoEq(char *c1, char *c2) {
    char host1[NAME_SIZE], host2[NAME_SIZE];
    int port1, port2;
    char dbname1[NAME_SIZE], dbname2[NAME_SIZE];
    char user1[NAME_SIZE], user2[NAME_SIZE];
    if (!dbp_conninfoParse(c1, host1, &port1, dbname1, user1, NAME_SIZE)) {
        return 0;
    }
    if (!dbp_conninfoParse(c2, host2, &port2, dbname2, user2, NAME_SIZE)) {
        return 0;
    }
    if (strcmp(host1, host2) != 0) {
        return 0;
    }
    if (port1 != port2) {
        return 0;
    }
    if (strcmp(dbname1, dbname2) != 0) {
        return 0;
    }
    if (strcmp(user1, user2) != 0) {
        return 0;
    }
    return 1;
}