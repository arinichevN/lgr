
#ifndef LIBPAS_SPI_H
#define LIBPAS_SPI_H

#include "main.h"

int wiringPiSPIGetFd  (int channel) ;
int wiringPiSPIDataRW (int channel, unsigned char *data, int len) ;
int wiringPiSPISetup  (int channel, int speed) ;


#endif 

