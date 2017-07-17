#ifndef LIBPAS_REG_H
#define LIBPAS_REG_H

#include "acp/main.h"

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

enum {
    REG_OFF,
    REG_INIT,
    REG_DO,
    REG_DISABLE,
    REG_WAIT,
    REG_BUSY,
    REG_COOLER,
    REG_HEATER,
    REG_MODE_PID,
    REG_MODE_ONF
} StateReg;

extern char * reg_getStateStr(char state);

extern int reg_controlEM(EM *item, float output);

#endif

