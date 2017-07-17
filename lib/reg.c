
#include "reg.h"

char * reg_getStateStr(char state) {
    switch (state) {
        case REG_OFF:
            return "OFF";
        case REG_INIT:
            return "INIT";
        case REG_DO:
            return "DO";
        case REG_WAIT:
            return "WAIT";
        case REG_BUSY:
            return "BUSY";
        case REG_COOLER:
            return "COOLER";
        case REG_HEATER:
            return "HEATER";
        case REG_DISABLE:
            return "DISABLE";
        case REG_MODE_PID:
            return "PID";
        case REG_MODE_ONF:
            return "ONF";
    }
    return "";
}

int reg_controlEM(EM *item, float output) {
    if (item == NULL) {
        return 0;
    }
    return acp_setEMDutyCycleR(item, output);
}