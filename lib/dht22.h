
#ifndef LIBPAS_DHT22_H
#define LIBPAS_DHT22_H

#include <stdint.h>
#include "timef.h"
#include "gpio.h"

#define MAXTIMINGS 85
#define MAX_VAL 83

extern int dht22_read(int pin, float *t, float *h);

#endif

