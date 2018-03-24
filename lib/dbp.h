#ifndef LIBPAS_DBP_H
#define LIBPAS_DBP_H
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

#include "app.h"

extern int dbp_init(PGconn **conn, char *conninfo);

extern void dbp_free(PGconn *conn);

extern int dbp_cmd(PGconn *conn, char *q);

extern PGresult *dbp_exec(PGconn *conn, char *q);

extern int dbp_getInt(int *item, PGconn *conn, char *q);

extern int dbp_conninfoParse(const char *buf, char *host, int *port, char *dbname, char *user, size_t str_size);

extern int dbp_conninfoEq(char *c1, char *c2);

#endif 

