/*
 *  pulse-width modulation 
 */
#include "pwm.h"

int pwmctl(PWM *item, int duty_cycle) {
    struct timespec now, dif, time_req;
    if (item->period.tv_sec < 0) {
#ifdef MODE_DEBUG
        printf("pwmctl: old period=%ld sec, and now period=0", item->period.tv_sec);
#endif
        item->period.tv_sec = 0;
        item->period.tv_nsec = 0;
        return (PWM_IDLE);
    } else if (item->period.tv_sec == 0) {
        return (PWM_IDLE);
    }
    //duty_cycle normalization
    if (duty_cycle < 0) {
        duty_cycle = 0;
    } else if (duty_cycle > item->rsl) {
        duty_cycle = item->rsl;
    }

    now = getCurrentTime();
    if (!item->initialized) {
        item->state = PWM_BUSY;
        item->start_time = now;
        item->initialized = 1;
    }
    switch (item->state) {
        case PWM_BUSY:
        {
            timespecsub(&now, &item->start_time, &dif);
            double d_duty_cycle, d_period, d_rsl, d_rest;
            d_duty_cycle = (double) duty_cycle;
            d_period = item->period.tv_sec;
            d_rsl = item->rsl;
            time_req.tv_sec = (duty_cycle * item->period.tv_sec) / item->rsl;
            d_rest = (d_duty_cycle * d_period) / d_rsl;
            double d1 = time_req.tv_sec;
            d_rest = d_rest - d1;
            d_rest = d_rest * 1000000000;
            time_req.tv_nsec = ((int) d_rest) + (duty_cycle * item->period.tv_nsec) / item->rsl;
            if (timespeccmp(&time_req, &dif, <=)) {
                if (duty_cycle != item->rsl) {
                    item->state = PWM_IDLE;
                    item->start_time = now;
                }
            }
            break;
        }
        case PWM_IDLE:
        {
            timespecsub(&now, &item->start_time, &dif);
            double d_duty_cycle, d_period, d_rsl, d_rest;
            d_duty_cycle = (double) duty_cycle;
            d_period = item->period.tv_sec;
            d_rsl = item->rsl;
            time_req.tv_sec = ((item->rsl - duty_cycle) * item->period.tv_sec) / item->rsl;
            d_rest = ((d_rsl - d_duty_cycle) * d_period) / d_rsl;
            double d1 = time_req.tv_sec;
            d_rest = d_rest - d1;
            d_rest = d_rest * 1000000000;
            time_req.tv_nsec = ((int) d_rest) + ((item->rsl - duty_cycle) * item->period.tv_nsec) / item->rsl;
            if (timespeccmp(&time_req, &dif, <=)) {
                if (duty_cycle != 0) {
                    item->state = PWM_BUSY;
                    item->start_time = now;
                }
            }
            break;
        }
        default:
            item->state = PWM_BUSY;
            break;
    }
#ifdef MODE_DEBUG
  // printf("%ld s %ld ns\n",time_req.tv_sec, time_req.tv_nsec); 
#endif
    return item->state;
}

