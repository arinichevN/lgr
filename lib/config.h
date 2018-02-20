
#ifndef LIBPAS_CONFIG_H
#define LIBPAS_CONFIG_H

#include "db.h"
#include "acp/main.h"
#include "udp.h"
#include "timef.h"

extern int config_getStrValFromTbl(PGconn *db_conn, const char *id, char *value, const char *tbl, size_t value_size);

extern int config_getBufSize(PGconn *db_conn, const char *id, size_t *value) ;

extern int config_getCycleDurationUs(PGconn *db_conn, const char *id, struct timespec *value) ;

extern int config_getUDPPort(PGconn *db_conn, const char *id, size_t *value) ;

extern int config_getPidPath(PGconn *db_conn, const char *id, char *value, size_t value_size) ;

extern int config_getI2cPath(PGconn *db_conn, const char *id, char *value, size_t value_size) ;

extern int config_getDbConninfo(PGconn *db_conn, const char *id, char *value, size_t value_size) ;

extern int config_checkPeerList(const PeerList *list);
extern int config_getPeerList(PGconn *db_conn, PeerList *list, int *fd);

extern int config_getLockKey(PGconn *db_conn, const char *id, char *value, size_t value_size);

#endif 

