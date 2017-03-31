
#ifndef LIBPAS_CONFIG_H
#define LIBPAS_CONFIG_H

#include "dbl.h"
#include "main.h"
#include "acp/main.h"
//#include "udp.h"
#include "timef.h"

typedef struct {
    PeerList *list;
    int *fd;
} PeerData;

typedef struct {
    SensorFTS *sensor;
    const PeerList *peer_list;
} SensorFTSData;

typedef struct {
    EM *em;
    const PeerList *peer_list;
} EMData;

extern int config_getPeerList(PeerList *list, int *fd, const char *db_path) ;

extern int config_getPeer(Peer *item, char * peer_id, int *fd,sqlite3 *db) ;

extern int config_getSensorFTS(SensorFTS *item, int sensor_id, const PeerList *pl, sqlite3 *db);

extern int config_getEM(EM *item, int em_id, const PeerList *pl, sqlite3 *db);

extern int config_getPhoneNumberListG(S1List *list, int group_id, const char *db_path);

extern int config_getPhoneNumberListO(S1List *list, const char *db_path);

#endif 

