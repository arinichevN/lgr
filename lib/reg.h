#ifndef LIBPAS_REG_H
#define LIBPAS_REG_H

#include "acp/main.h"
#include "dbl.h"

#define REG_MODE_PID_STR "pid"
#define REG_MODE_ONF_STR "onf"

#define SNSR_VAL item->sensor.value.value
#define SNSR_TM item->sensor.value.tm

#define VAL_IS_OUT_H SNSR_VAL > item->goal + item->heater.delta
#define VAL_IS_OUT_C SNSR_VAL < item->goal - item->cooler.delta
#define SNSRF_COUNT_MAX 7

#define REG_EM_MODE_COOLER_STR "cooler"
#define REG_EM_MODE_HEATER_STR "heater"
#define REG_EM_MODE_BOTH_STR "both"

typedef struct {
    int id;
    int active;
    struct timespec timeout;
    float heater_duty_cycle;
    float cooler_duty_cycle;
    Ton_ts tmr;
    int done;
} RegSecure;

DEC_LIST(RegSecure)

enum {
    REG_OFF,
    REG_INIT,
    REG_DO,
    REG_DISABLE,
    REG_WAIT,
    REG_BUSY,
    REG_SECURE,
    REG_COOLER,
    REG_HEATER,
    REG_MODE_PID,
    REG_MODE_ONF
} StateReg;

extern char * reg_getStateStr(char state);

extern int reg_controlEM(EM *item, float output);

extern int reg_getSecureFDB(RegSecure *item, int id, sqlite3 *dbl, const char *db_path);

extern int reg_secureNeed(RegSecure *item);

extern void reg_secureTouch(RegSecure *item);

#endif

