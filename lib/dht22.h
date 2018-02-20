
#ifndef LIBPAS_DHT22_H
#define LIBPAS_DHT22_H

#include <stdint.h>

#include "timef.h"
#include "gpio.h"

extern int dht22_read(int pin, float *t, float *h);

#endif

