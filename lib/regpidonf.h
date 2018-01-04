#ifndef LIBPAS_REGPIDONF_H
#define LIBPAS_REGPIDONF_H

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

} RegPIDOnfEM;

typedef struct {
    SensorFTS sensor;
    EM em;
    
    float delta;
    PID pid;
    char mode;//heater or cooler
    float goal;

    char state;
    char state_r;
    char state_onf;
    float output;
    int snsrf_count;
} RegPIDOnf;

extern void regpidonf_control(RegPIDOnf *item) ;

extern void regpidonf_enable(RegPIDOnf *item) ;

extern void regpidonf_disable(RegPIDOnf *item) ;

extern int regpidonf_getEnabled(const RegPIDOnf *item) ;

extern void regpidonf_setDelta(RegPIDOnf *item, float value) ;

extern void regpidonf_setKp(RegPIDOnf *item, float value) ;

extern void regpidonf_setKi(RegPIDOnf *item, float value) ;

extern void regpidonf_setKd(RegPIDOnf *item, float value) ;

extern void regpidonf_setGoal(RegPIDOnf *item, float value) ;

extern void regpidonf_setMode(RegPIDOnf *item, const char * value) ;

extern void regpidonf_setEMMode(RegPIDOnf *item, const char * value) ;

extern void regpidonf_setPower(RegPIDOnf *item, float value) ;

extern void regpidonf_turnOff(RegPIDOnf *item) ;

extern int regpidonf_check(const RegPIDOnf *item) ;

#endif 

