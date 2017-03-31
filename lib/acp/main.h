#ifndef LIBPAS_ACP_MAIN_H
#define LIBPAS_ACP_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#include "app.h"
#include "cmd.h"

#include "../app.h"
#include "../timef.h"
#include "../util.h"
#include "../udp.h"
#include "../crc.h"
#include "gwu59.h"

#define ACP_RETRY_NUM 12

typedef struct {
    char id[NAME_SIZE];
    int *fd;
    struct sockaddr_in addr;
    socklen_t addr_size;
    int active;
    struct timespec time1;
    Mutex mutex;
} Peer;

DEF_LIST(Peer)

typedef struct {
    int *item;
    size_t length;
} I1List;

typedef struct {
    int p0;
    int p1;
} I2;

DEF_LIST(I2)

typedef struct {
    int p0;
    int p1;
    int p2;
} I3;

DEF_LIST(I3)

typedef struct {
    float *item;
    size_t length;
} F1List;

typedef struct {
    int p0;
    float p1;
} I1F1;

DEF_LIST(I1F1)

typedef struct {
    char *item;
    size_t length;
} S1List;

typedef struct {
    int p0;
    char p1[LINE_SIZE];
} I1S1;

DEF_LIST(I1S1)

typedef struct {
    char p0[LINE_SIZE];
    char p1[LINE_SIZE];
} S2;

DEF_LIST(S2)

typedef struct {
    int id;
    float value;
    struct timespec tm;
    int state;
} FTS;

DEF_LIST(FTS)

typedef struct {
    int id;
    int remote_id;
    Peer *source;
    int value;
    Mutex mutex;
    struct timespec last_read_time;
    struct timespec interval_min;
    int last_return;
} SensorInt;

DEF_LIST(SensorInt)

typedef struct {
    int id;
    int remote_id;
    Peer *source;
    FTS value;
    Mutex mutex;
    struct timespec last_read_time;
    struct timespec interval_min;
    int last_return;
} SensorFTS;

DEF_LIST(SensorFTS)

typedef struct {
    int id;
    int remote_id;
    Peer *source;
    float last_output; //we will keep last output value in order not to repeat the same queries to peers
    float pwm_rsl; //max duty cycle value (see lib/pid.h PWM_RSL)
    Mutex mutex;
} EM; //executive mechanism

DEF_LIST(EM)

#define ACP_HEADER_LENGTH 3

#define ACP_RESP_BUF_SIZE_MIN 6UL

#define ACP_QUANTIFIER_BROADCAST '!'
#define ACP_QUANTIFIER_SPECIFIC '.'

#define ACP_RESP_REQUEST_FAILED ("F")
#define ACP_RESP_REQUEST_SUCCEEDED ("T")
#define ACP_RESP_REQUEST_SUCCEEDED_PARTIAL ("P")
#define ACP_RESP_RESULT_UNKNOWN ("R")
#define ACP_RESP_COMMAND_UNKNOWN ("U")
#define ACP_RESP_QUANTIFIER_UNKNOWN ("Q")
#define ACP_RESP_CRC_ERROR ("C")
#define ACP_RESP_BUF_OVERFLOW ("O")

DEF_FUN_LIST_GET_BY_IDSTR(Peer)

DEF_FUN_LIST_GET_BY_ID(SensorFTS)

DEF_FUN_LIST_GET_BY_ID(EM)

extern int acp_initBuf(char *buf, size_t buf_size) ;

extern void acp_parsePackI1( char *buf, I1List *list, size_t list_max_size) ;

extern void acp_parsePackI2( char *buf, I2List *list, size_t list_max_size) ;

extern void acp_parsePackI3( char *buf, I3List *list, size_t list_max_size) ;

extern void acp_parsePackF1( char *buf, F1List *list, size_t list_max_size) ;

extern void acp_parsePackI1F1( char *buf, I1F1List *list, size_t list_max_size) ;

extern void acp_parsePackS1( char *buf, S1List *list, size_t list_max_size) ;

extern void acp_parsePackI1S1( char *buf, I1S1List *list, size_t list_max_size) ;

extern void acp_parsePackFTS( char *buf, FTSList *list, size_t list_max_size) ;

extern void acp_parsePackS2( char *buf, S2List *list, size_t list_max_size) ;

extern size_t acp_packlen(char *buf, size_t buf_size) ;

extern int acp_bufAddHeader(char *buf, char qnf, char *cmd_str, size_t buf_size) ;

extern int acp_bufAddFooter(char *buf, size_t buf_size) ;

extern int acp_sendBuf(char *buf, size_t buf_size, Peer *peer);

extern int acp_sendBufArrPackI1List(char cmd, size_t buf_size, const I1List *data, Peer *peer) ;

extern int acp_sendBufArrPackI2List(char cmd, size_t buf_size, const I2List *data, Peer *peer) ;

extern int acp_sendBufArrPackS2List(char cmd, size_t buf_size, const S2List *data, Peer *peer);

extern int acp_sendBufArrPackS1List(char cmd, size_t buf_size, const S1List *data, Peer *peer);

extern void acp_sendStr(const char *s, uint8_t *crc, Peer *peer) ;

extern void acp_sendFooter(int8_t crc, Peer *peer) ;

extern int acp_sendBufPack(char *buf, char qnf,  char *cmd_str, size_t buf_size, Peer *peer) ;

extern int acp_sendStrPack(char qnf, char *cmd, size_t buf_size, Peer *peer) ;

extern int acp_bufCatDate(struct tm *date, char *buf, size_t buf_size);

extern int acp_recvOK(Peer *peer, size_t buf_size) ;

extern char acp_recvPing(Peer *peer, size_t buf_size);

extern int acp_recvFTS(FTSList *list, char qnf, char *cmd, size_t buf_size, size_t list_max_size, int fd) ;

extern int acp_recvI2(I2List *list, char qnf, char *cmd, size_t buf_size, size_t list_max_size, int fd);

extern void freePeer(PeerList *list);

extern void acp_printI1(I1List *list) ;

extern void acp_printI2(I2List *list) ;

extern void acp_printI3(I3List *list) ;

DEF_FUN_LOCK(SensorInt)

DEF_FUN_LOCK(SensorFTS)

DEF_FUN_LOCK(Peer)

DEF_FUN_LOCK(EM)

DEF_FUN_UNLOCK(SensorInt)

DEF_FUN_UNLOCK(SensorFTS)

DEF_FUN_UNLOCK(Peer)

DEF_FUN_UNLOCK(EM)

extern int acp_setEMOutput(EM *em, int output, size_t udp_buf_size);

extern int acp_setEMDutyCycle(EM *em, float output, size_t udp_buf_size);

extern int acp_setEMOutputR(EM *em, int output, size_t udp_buf_size);

extern int acp_setEMDutyCycleR(EM *em, float output, size_t udp_buf_size);

extern int acp_readSensorInt(SensorInt *s, size_t udp_buf_size);

extern int acp_readSensorFTS(SensorFTS *s, size_t udp_buf_size);

extern void acp_pingPeerList( PeerList *list,struct timespec interval, struct timespec now, size_t udp_buf_size);

extern void acp_waitUnlock(Peer *item, char *cmd_unlock,char *cmd_check, unsigned int interval_us, size_t udp_buf_size) ;

extern int acp_makeCall(Peer *peer, char *phone, size_t udp_buf_size) ;

extern int acp_sendSMS(Peer *peer, char *phone, char *message, size_t udp_buf_size) ;

extern int acp_sendCmdGetInt(Peer *item, char* cmd, int *output, size_t udp_buf_size) ;

extern void acp_pingPeer(Peer *item, size_t udp_buf_size);
#endif /* ACP_H */

