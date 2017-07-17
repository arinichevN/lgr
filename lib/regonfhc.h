#ifndef LIBPAS_REGONFHC_H
#define LIBPAS_REGONFHC_H

#include "timef.h"
#include "acp/main.h"
#include "reg.h"

typedef struct {
    EM em;
    float delta;
    float output;
    int use;
} RegOnfHCEM;

typedef struct {
    SensorFTS sensor;

    RegOnfHCEM heater;
    RegOnfHCEM cooler;
    float goal;
    struct timespec change_gap;

    char state;
    char state_r;
    char state_onf;
    float output;
    int snsrf_count;
    Ton_ts tmr;
} RegOnfHC;

extern void regonfhc_control(RegOnfHC *item);

extern void regonfhc_enable(RegOnfHC *item);

extern void regonfhc_disable(RegOnfHC *item);

extern void regonfhc_setCoolerDelta(RegOnfHC *item, float value);

extern void regonfhc_setHeaterDelta(RegOnfHC *item, float value);

extern void regonfhc_setGoal(RegOnfHC *item, float value);

extern void regonfhc_setChangeGap(RegOnfHC *item, int value);

extern void regonfhc_setHeaterPower(RegOnfHC *item, float value);

extern void regonfhc_setCoolerPower(RegOnfHC *item, float value);

extern void regonfhc_setEMMode(RegOnfHC *item, const char * value);

extern void regonfhc_turnOff(RegOnfHC *item);

#endif 

