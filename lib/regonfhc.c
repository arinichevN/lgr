
#include "regonfhc.h"
#include "reg.c"

static void controlEM(RegOnfHCEM *item, float output) {
    if (item->use) {
        reg_controlEM(&item->em, output);
        item->output = output;
    }
}

void regonfhc_control(RegOnfHC *item) {
    switch (item->state) {
        case REG_INIT:
            if (!acp_readSensorFTS(&item->sensor)) {
#ifdef MODE_DEBUG
                puts("reading from sensor failed");
#endif
                return;
            }
            item->tmr.ready = 0;
            item->state_r = REG_HEATER;
            controlEM(&item->heater, 0.0f);
            controlEM(&item->cooler, 0.0f);
            item->output = 0.0f;
            item->snsrf_count = 0;
            item->state_onf = REG_WAIT;
            item->state = REG_BUSY;
            if (item->heater.use && item->cooler.use) {
                    if (SNSR_VAL > item->goal) {
                        item->state_r = REG_COOLER;
                    } else {
                        item->state_r = REG_HEATER;
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
                RegOnfHCEM *reg_em = NULL;
                RegOnfHCEM *reg_em_other = NULL;
                switch (item->state_r) {
                    case REG_HEATER:
                        if (VAL_IS_OUT_H) {
                            value_is_out = 1;
                        }
                        other_em = REG_COOLER;
                        reg_em = &item->heater;
                        reg_em_other = &item->cooler;
                        break;
                    case REG_COOLER:
                        if (VAL_IS_OUT_C) {
                            value_is_out = 1;
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
                controlEM(reg_em, item->output);
                controlEM(reg_em_other, 0.0f);
                if(reg_secureNeed(&item->secure_out)){
                    item->state=REG_SECURE;
                }
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
        case REG_SECURE:
            controlEM(&item->heater, item->secure_out.heater_duty_cycle);
            controlEM(&item->cooler,item->secure_out.cooler_duty_cycle);
            if(!reg_secureNeed(&item->secure_out)){
                item->state=REG_BUSY;
            }
            break;
        case REG_DISABLE:
            controlEM(&item->heater, 0.0f);
            controlEM(&item->cooler, 0.0f);
            item->heater.output = 0.0f;
            item->cooler.output = 0.0f;
            item->output = 0.0f;
            item->tmr.ready = 0;
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
    struct timespec tm1 = getTimeRestTmr(item->change_gap, item->tmr);
    printf("state=%s state_onf=%s EM_state=%s goal=%.1f delta_h/c=%.1f/%.1f real=%.1f real_st=%d out=%.1f change_time=%ldsec\n", state, state_onf, state_r, item->goal, item->heater.delta, item->cooler.delta, SNSR_VAL, item->sensor.value.state, item->output, tm1.tv_sec);
#endif
}

void regonfhc_enable(RegOnfHC *item) {
    item->state = REG_INIT;
}

void regonfhc_disable(RegOnfHC *item) {
    item->state = REG_DISABLE;
}

int regonfhc_getEnabled(const RegOnfHC *item) {
    if(item->state==REG_DISABLE || item->state==REG_OFF){
        return 0;
    }
    return 1;
}

void regonfhc_setCoolerDelta(RegOnfHC *item, float value) {
    item->cooler.delta = value;
    if (item->state == REG_BUSY && item->state_r == REG_COOLER) {
        item->state = REG_INIT;
    }
}

void regonfhc_setHeaterDelta(RegOnfHC *item, float value) {
    item->heater.delta = value;
    if (item->state == REG_BUSY && item->state_r == REG_HEATER) {
        item->state = REG_INIT;
    }
}

void regonfhc_setGoal(RegOnfHC *item, float value) {
    item->goal = value;
/*
    if (item->state == REG_BUSY) {
        item->state = REG_INIT;
    }
*/
}

void regonfhc_setChangeGap(RegOnfHC *item, int value) {
    item->change_gap.tv_sec = value;
    item->change_gap.tv_nsec = 0;
}

void regonfhc_setEMMode(RegOnfHC *item, const char * value) {
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

void regonfhc_setHeaterPower(RegOnfHC *item, float value) {
    controlEM(&item->heater, value);
    item->output = item->heater.output;
}

void regonfhc_setCoolerPower(RegOnfHC *item, float value) {
    controlEM(&item->cooler, value);
    item->output = item->cooler.output;
}

void regonfhc_turnOff(RegOnfHC *item) {
    item->state = REG_OFF;
    controlEM(&item->cooler, 0.0f);
    controlEM(&item->heater, 0.0f);
}

void regonfhc_secureOutTouch(RegOnfHC *item){
     reg_secureTouch(&item->secure_out);
}