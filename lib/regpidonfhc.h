#ifndef LIBPAS_REGPIDONFHC_H
#define LIBPAS_REGPIDONFHC_H

#include "timef.h"
#include "acp/main.h"
#include "reg.h"
#include "pid.h"

typedef struct {
    EM em;
    float delta;
    PID pid;
    float output;
    char mode;
    int use;
} RegPIDOnfHCEM;

typedef struct {
    SensorFTS sensor;

    RegPIDOnfHCEM heater;
    RegPIDOnfHCEM cooler;
    float goal;
    struct timespec change_gap;

    char state;
    char state_r;
    char state_onf;
    float output;
    int snsrf_count;
    Ton_ts tmr;
} RegPIDOnfHC;

extern void regpidonfhc_control(RegPIDOnfHC *item);

extern void regpidonfhc_enable(RegPIDOnfHC *item);

extern void regpidonfhc_disable(RegPIDOnfHC *item);

extern void regpidonfhc_setCoolerDelta(RegPIDOnfHC *item, float value);

extern void regpidonfhc_setHeaterDelta(RegPIDOnfHC *item, float value);

extern void regpidonfhc_setHeaterKp(RegPIDOnfHC *item, float value);

extern void regpidonfhc_setHeaterKi(RegPIDOnfHC *item, float value);

extern void regpidonfhc_setHeaterKd(RegPIDOnfHC *item, float value);

extern void regpidonfhc_setCoolerKp(RegPIDOnfHC *item, float value);

extern void regpidonfhc_setCoolerKi(RegPIDOnfHC *item, float value);

extern void regpidonfhc_setCoolerKd(RegPIDOnfHC *item, float value);

extern void regpidonfhc_setGoal(RegPIDOnfHC *item, float value);

extern void regpidonfhc_setEMMode(RegPIDOnfHC *item, const char * value);

extern void regpidonfhc_setHeaterMode(RegPIDOnfHC *item, const char * value);

extern void regpidonfhc_setCoolerMode(RegPIDOnfHC *item, const char * value);

extern void regpidonfhc_setChangeGap(RegPIDOnfHC *item, int value);

extern void regpidonfhc_setHeaterPower(RegPIDOnfHC *item, float value);

extern void regpidonfhc_setCoolerPower(RegPIDOnfHC *item, float value);

extern void regpidonfhc_turnOff(RegPIDOnfHC *item);

#endif 

