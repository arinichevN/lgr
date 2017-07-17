
#ifndef LGR_H
#define LGR_H

#include "lib/dbl.h"
#include "lib/configl.h"
#include "lib/util.h"
#include "lib/crc.h"
#include "lib/app.h"
#include "lib/timef.h"
#include "lib/udp.h"
#include "lib/acp/main.h"
#include "lib/acp/app.h"
#include "lib/acp/lgr.h"


#define APP_NAME lgr
#define APP_NAME_STR TOSTRING(APP_NAME)


#ifdef MODE_FULL
#define CONF_DIR "/etc/controller/" APP_NAME_STR "/"
#endif
#ifndef MODE_FULL
#define CONF_DIR "./"
#endif
#define CONFIG_FILE "" CONF_DIR "config.tsv"
#define CONFIG_FILE_DB "" CONF_DIR "main.conf"

#define WAIT_RESP_TIMEOUT 3

#define LOG_KIND_FTS "fts"
#define STATUS_SUCCESS "SUCCESS"
#define STATUS_FAILURE "FAILURE"

#define PROG_LIST_LOOP_DF Prog *curr = prog_list.top;
#define PROG_LIST_LOOP_ST while (curr != NULL) {
#define PROG_LIST_LOOP_SP curr = curr->next; } curr = prog_list.top;
#define PROG_FIELDS "id, sensor_fts_id, kind, interval_min, max_rows, enable,load"

enum {
    OFF,
    INIT,
    ACT,
    DISABLE
} StateAPP;

struct prog_st {
    int id;
    int remote_id;
    struct timespec interval_min;
    size_t max_rows;
    char kind[NAME_SIZE];

    unsigned int retry_count;
    char state;
    Ton_ts tmr;
    SensorFTS sensor_fts;

    Mutex mutex;
    struct prog_st *next;
};

typedef struct prog_st Prog;

DEF_LLIST(Prog)


//typedef struct {
//    PeerList *list;
//    int *fd;
//} PeerData;
//
//typedef struct {
//    SensorFTS *sensor;
//    PeerList *peer_list;
//} SensorFTSData;

typedef struct {
    sqlite3 *db;
    PeerList *peer_list;
    ProgList *prog_list;
} ProgData;

typedef struct {
    sqlite3 *db;
    Prog *item;
    PeerList *peer_list;
    ProgList *prog_list;
} ProgDataR;

extern int readSettings() ;

extern int initData() ;

extern void initApp() ;

extern void serverRun(int *state, int init_state) ;

extern int saveFTS(Prog *item, const char *db_path) ;

extern int clearLog(int dev_id, sqlite3 *db) ;

extern void progControl(Prog *item) ;

extern void *threadFunction(void *arg) ;

extern int createThread_ctl() ;

extern void freeProg(ProgList *list) ;

extern void freeData() ;

extern void freeApp() ;

extern void exit_nicely() ;

extern void exit_nicely_e(char *s) ;
#endif 

