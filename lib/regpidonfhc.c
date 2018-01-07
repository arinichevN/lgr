
#include "regpidonfhc.h"
#include "reg.c"
#include "regonfhc.h"

static void controlEM(RegPIDOnfHCEM *item, float output) {
    if (item->use) {
        reg_controlEM(&item->em, output);
        item->output = output;
    }
}

void regpidonfhc_control(RegPIDOnfHC *item) {
    switch (item->state) {
        case REG_INIT:
            item->tmr.ready = 0;
            controlEM(&item->heater, 0.0f);
            controlEM(&item->cooler, 0.0f);
            item->output = 0.0f;
            item->snsrf_count = 0;
            item->heater.pid.mode = PID_MODE_HEATER;
            item->cooler.pid.mode = PID_MODE_COOLER;
            item->heater.pid.reset = 1;
            item->cooler.pid.reset = 1;
            item->state_onf = REG_WAIT;
            item->state = REG_BUSY;
            if (item->heater.use && item->cooler.use) {
                if (acp_readSensorFTS(&item->sensor)) {
                    if (SNSR_VAL > item->goal) {
                        item->state_r = REG_COOLER;
                    } else {
                        item->state_r = REG_HEATER;
                    }
                }
            } else if (item->heater.use && !item->cooler.use) {
                item->state_r = REG_HEATER;
            } else if (!item->heater.use && item->cooler.use) {
                item->state_r = REG_COOLER;
            } else {
                item->state = REG_DISABLE;
            }
            break;
        case REG_BUSY:
        {
            if (acp_readSensorFTS(&item->sensor)) {
                item->snsrf_count = 0;
                int value_is_out = 0;
                char other_em;
                RegPIDOnfHCEM *reg_em = NULL;
                RegPIDOnfHCEM *reg_em_other = NULL;
                switch (item->state_r) {
                    case REG_HEATER:
                        switch (item->heater.mode) {
                            case REG_MODE_PID:
                                if (item->output < 0) {
                                    value_is_out = 1;
                                }
                                break;
                            case REG_MODE_ONF:
                                if (VAL_IS_OUT_H) {
                                    value_is_out = 1;
                                }
                                break;
                        }
                        other_em = REG_COOLER;
                        reg_em = &item->heater;
                        reg_em_other = &item->cooler;
                        break;
                    case REG_COOLER:
                        switch (item->cooler.mode) {
                            case REG_MODE_PID:
                                if (item->output < 0) {
                                    value_is_out = 1;
                                }
                                break;
                            case REG_MODE_ONF:
                                if (VAL_IS_OUT_C) {
                                    value_is_out = 1;
                                }
                                break;
                        }
                        other_em = REG_HEATER;
                        reg_em = &item->cooler;
                        reg_em_other = &item->heater;
                        break;
                }
                if (value_is_out && reg_em_other->use) {
                    if (ton_ts(item->change_gap, &item->tmr)) {
#ifdef MODE_DEBUG
                        char *state1 = reg_getStateStr(item->state_r);
                        char *state2 = reg_getStateStr(other_em);
                        printf("state_r switched from %s to %s\n", state1, state2);
#endif
                        item->state_r = other_em;
                        controlEM(reg_em, 0.0f);
                    }
                } else {
                    item->tmr.ready = 0;
                }

                switch (item->state_r) {
                    case REG_HEATER:
                        reg_em = &item->heater;
                        reg_em_other = &item->cooler;
                        break;
                    case REG_COOLER:
                        reg_em = &item->cooler;
                        reg_em_other = &item->heater;
                        break;
                }
                switch (reg_em->mode) {
                    case REG_MODE_PID:
                        item->output = pid(&reg_em->pid, item->goal, item->sensor.value.value);
                        break;
                    case REG_MODE_ONF:
                        switch (item->state_onf) {
                            case REG_DO:
                                switch (item->state_r) {
                                    case REG_HEATER:
                                        if (SNSR_VAL > item->goal + item->heater.delta) {
                                            item->state_onf = REG_WAIT;
                                        }
                                        break;
                                    case REG_COOLER:
                                        if (SNSR_VAL < item->goal - item->cooler.delta) {
                                            item->state_onf = REG_WAIT;
                                        }
                                        break;
                                }
                                item->output = reg_em->em.pwm_rsl;
                                break;
                            case REG_WAIT:
                                switch (item->state_r) {
                                    case REG_HEATER:
                                        if (SNSR_VAL < item->goal - item->heater.delta) {
                                            item->state_onf = REG_DO;
                                        }
                                        break;
                                    case REG_COOLER:
                                        if (SNSR_VAL > item->goal + item->cooler.delta) {
                                            item->state_onf = REG_DO;
                                        }
                                        break;
                                }
                                item->output = 0.0f;
                                break;
                        }
                        break;
                }
                controlEM(reg_em, item->output);
                controlEM(reg_em_other, 0.0f);
            } else {
                if (item->snsrf_count > SNSRF_COUNT_MAX) {
                    controlEM(&item->heater, 0.0f);
                    controlEM(&item->cooler, 0.0f);
                    item->output = 0.0f;
                    item->state = REG_INIT;
#ifdef MODE_DEBUG
                    puts("reading from sensor failed, EM turned off");
#endif
                } else {
                    item->snsrf_count++;
#ifdef MODE_DEBUG
                    printf("sensor failure counter: %d\n", item->snsrf_count);
#endif
                }
            }
            break;
        }
        case REG_DISABLE:
            controlEM(&item->heater, 0.0f);
            controlEM(&item->cooler, 0.0f);
            item->heater.output = 0.0f;
            item->cooler.output = 0.0f;
            item->tmr.ready = 0;
            item->output = 0.0f;
            item->state_r = REG_OFF;
            item->state_onf = REG_OFF;
            item->state = REG_OFF;
            break;
        case REG_OFF:
            break;
        default:
            item->state = REG_OFF;
            break;
    }
#ifdef MODE_DEBUG
    char *state = reg_getStateStr(item->state);
    char *state_r = reg_getStateStr(item->state_r);
    char *state_onf = reg_getStateStr(item->state_onf);
    char *heater_mode = reg_getStateStr(item->heater.mode);
    char *cooler_mode = reg_getStateStr(item->cooler.mode);
    struct timespec tm1 = getTimeRestTmr(item->change_gap, item->tmr);
    printf("state=%s state_onf=%s EM_state=%s hmode=%s cmode=%s goal=%.1f real=%.1f out=%.1f change_time=%ldsec\n", state, state_onf, state_r,heater_mode, cooler_mode, item->goal, SNSR_VAL, item->output, tm1.tv_sec);
#endif
}

void regpidonfhc_enable(RegPIDOnfHC *item) {
    item->state = REG_INIT;
}

void regpidonfhc_disable(RegPIDOnfHC *item) {
    item->state = REG_DISABLE;
}

int regpidonfhc_getEnabled(const RegPIDOnfHC *item) {
    if(item->state==REG_DISABLE || item->state==REG_OFF){
        return 0;
    }
    return 1;
}

void regpidonfhc_setCoolerDelta(RegPIDOnfHC *item, float value) {
    item->cooler.delta = value;
    if (item->state==REG_BUSY && item->heater.mode == REG_MODE_ONF && item->state_r == REG_COOLER) {
        item->state = REG_INIT;
    }
}

void regpidonfhc_setHeaterDelta(RegPIDOnfHC *item, float value) {
    item->heater.delta = value;
    if (item->state==REG_BUSY && item->heater.mode == REG_MODE_ONF && item->state_r == REG_HEATER) {
        item->state = REG_INIT;
    }
}

void regpidonfhc_setHeaterKp(RegPIDOnfHC *item, float value) {
    item->heater.pid.kp = value;
}

void regpidonfhc_setHeaterKi(RegPIDOnfHC *item, float value) {
    item->heater.pid.ki = value;
}

void regpidonfhc_setHeaterKd(RegPIDOnfHC *item, float value) {
    item->heater.pid.kd = value;
}

void regpidonfhc_setCoolerKp(RegPIDOnfHC *item, float value) {
    item->cooler.pid.kp = value;
}

void regpidonfhc_setCoolerKi(RegPIDOnfHC *item, float value) {
    item->cooler.pid.ki = value;
}

void regpidonfhc_setCoolerKd(RegPIDOnfHC *item, float value) {
    item->cooler.pid.kd = value;
}

void regpidonfhc_setGoal(RegPIDOnfHC *item, float value) {
    item->goal = value;
/*
    if (item->state == REG_BUSY) {
        item->state = REG_INIT;
    }
*/
}

void regpidonfhc_setHeaterMode(RegPIDOnfHC *item, const char * value) {
    if (strncmp(value, REG_MODE_PID_STR, 3) == 0) {
        item->heater.mode = REG_MODE_PID;
    } else if (strncmp(value, REG_MODE_ONF_STR, 3) == 0) {
        item->heater.mode = REG_MODE_ONF;
    } else {
        return;
    }
    if (item->state == REG_BUSY && item->state_r==REG_HEATER) {
        item->state = REG_INIT;
    }
}

void regpidonfhc_setCoolerMode(RegPIDOnfHC *item, const char * value) {
    if (strncmp(value, REG_MODE_PID_STR, 3) == 0) {
        item->cooler.mode = REG_MODE_PID;
    } else if (strncmp(value, REG_MODE_ONF_STR, 3) == 0) {
        item->cooler.mode = REG_MODE_ONF;
    } else {
        return;
    }
    if (item->state == REG_BUSY && item->state_r==REG_COOLER) {
        item->state = REG_INIT;
    }
}

void regpidonfhc_setEMMode(RegPIDOnfHC *item, const char * value) {
    if (strcmp(REG_EM_MODE_COOLER_STR, value) == 0) {
        controlEM(&item->heater, 0.0f);
        item->cooler.use = 1;
        item->heater.use = 0;
    } else if (strcmp(REG_EM_MODE_HEATER_STR, value) == 0) {
        controlEM(&item->cooler, 0.0f);
        item->cooler.use = 0;
        item->heater.use = 1;
    } else if (strcmp(REG_EM_MODE_BOTH_STR, value) == 0) {
        item->cooler.use = 1;
        item->heater.use = 1;
    } else {
        controlEM(&item->heater, 0.0f);
        controlEM(&item->cooler, 0.0f);
        item->cooler.use = 0;
        item->heater.use = 0;
    }
    if (item->state == REG_BUSY) {
        item->state = REG_INIT;
    }
}

void regpidonfhc_setChangeGap(RegPIDOnfHC *item, int value) {
    item->change_gap.tv_sec = value;
    item->change_gap.tv_nsec = 0;
}

void regpidonfhc_setHeaterPower(RegPIDOnfHC *item, float value) {
    controlEM(&item->heater, value);
    item->output = item->heater.output;
}

void regpidonfhc_setCoolerPower(RegPIDOnfHC *item, float value) {
    controlEM(&item->cooler, value);
    item->output = item->cooler.output;
}

void regpidonfhc_turnOff(RegPIDOnfHC *item) {
    item->state = REG_OFF;
    controlEM(&item->cooler, 0.0f);
    controlEM(&item->heater, 0.0f);
}