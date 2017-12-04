/*

#include "max31855.h"
 */

/*
int max31855_init(int sclk, int cs, int miso) {
    pinModeOut(cs);
    pinModeOut(sclk);
    pinModeIn(miso);
    pinPUD(miso, PUD_DOWN);
    pinPUD(sclk, PUD_OFF);
    pinPUD(cs, PUD_OFF);
    pinHigh(cs);
    pinLow(sclk);
    return 1;
}

static void printInt32(uint32_t d) {
    int i;
    for (i = 31; i >= 0; i--) {
        int v = (d >> i) & 1;
        printf("%d", v);
    }
    puts("");
}
#define DELAY 1000

int max31855_read(float *result, int sclk, int cs, int miso) {
    uint32_t v;
    pinLow(cs);
    delayUsBusy(DELAY);
    {
        int i;
        for (i = 31; i >= 0; i--) {
            pinHigh(sclk);
            delayUsBusy(DELAY);
            if (pinRead(miso)) {
                v |= (1 << i);
            }
            pinLow(sclk);
            delayUsBusy(DELAY);
        }
    }
    pinHigh(cs);
#ifdef MODE_DEBUG
    printInt32(v);
#endif
    int error = 0;
    if (v & 0x20000) {
#ifdef MODE_DEBUG
        fprintf(stderr, "max31855_read: warning: bit 18 should be 0 where sclk=%d and cs=%d and miso=%d\n", sclk, cs, miso);
#endif
    }
    if (v & 0x8) {
#ifdef MODE_DEBUG
        fprintf(stderr, "max31855_read: warning: bit 4 should be 0 where sclk=%d and cs=%d and miso=%d\n", sclk, cs, miso);
#endif
    }
    if (v & 0x4) {
#ifdef MODE_DEBUG
        fprintf(stderr, "max31855_read: thermocouple is short-circuited to VCC where sclk=%d and cs=%d and miso=%d\n", sclk, cs, miso);
#endif
        error = 1;
    }
    if (v & 0x2) {
#ifdef MODE_DEBUG
        fprintf(stderr, "max31855_read: thermocouple is short-circuited to GND where sclk=%d and cs=%d and miso=%d\n", sclk, cs, miso);
#endif
        error = 1;
    }
    if (v & 0x1) {
#ifdef MODE_DEBUG
        fprintf(stderr, "max31855_read: thermocouple input is open where sclk=%d and cs=%d and miso=%d\n", sclk, cs, miso);
#endif
        error = 1;
    }
    if (v & 0x8000) {
#ifdef MODE_DEBUG
        fprintf(stderr, "max31855_read: fault has been found where sclk=%d and cs=%d and miso=%d\n", sclk, cs, miso);
#endif
        error = 1;
    }
    if (error) {
        return 0;
    }
    if (v & 0x80000000) {
        v = 0xFFFFC000 | ((v >> 18) & 0x00003FFFF);
    } else {
        v >>= 18;
    }
 *result = v * 0.25;
    return 1;
}
 */

#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>
#include <linux/spi/spidev.h>

/*
#include "wiringPi.h"
 */

int wiringPiSPIGetFd(int channel);
int wiringPiSPIDataRW(int channel, unsigned char *data, int len);
int wiringPiSPISetupMode(int channel, int speed, int mode);
int wiringPiSPISetup(int channel, int speed);


// The SPI bus parameters
//	Variables as they need to be passed as pointers later on

static const char *spiDev0 = "/dev/spidev0.0";
static const char *spiDev1 = "/dev/spidev0.1";
static const uint8_t spiBPW = 8;
static const uint16_t spiDelay = 0;

static uint32_t spiSpeeds [2];
static int spiFds [2];

/*
 * wiringPiSPIGetFd:
 *	Return the file-descriptor for the given channel
 *********************************************************************************
 */

int wiringPiSPIGetFd(int channel) {
    return spiFds [channel & 1];
}

/*
 * wiringPiSPIDataRW:
 *	Write and Read a block of data over the SPI bus.
 *	Note the data ia being read into the transmit buffer, so will
 *	overwrite it!
 *	This is also a full-duplex operation.
 *********************************************************************************
 */

int wiringPiSPIDataRW(int channel, unsigned char *data, int len) {
    struct spi_ioc_transfer spi;

    channel &= 1;

    // Mentioned in spidev.h but not used in the original kernel documentation
    //	test program )-:

    memset(&spi, 0, sizeof (spi));

    spi.tx_buf = (unsigned long) data;
    spi.rx_buf = (unsigned long) data;
    spi.len = len;
    spi.delay_usecs = spiDelay;
    spi.speed_hz = spiSpeeds [channel];
    spi.bits_per_word = spiBPW;

    return ioctl(spiFds [channel], SPI_IOC_MESSAGE(1), &spi);
}

/*
 * wiringPiSPISetupMode:
 *	Open the SPI device, and set it up, with the mode, etc.
 *********************************************************************************
 */

int wiringPiSPISetupMode(int channel, int speed, int mode) {
    int fd;

    mode &= 3; // Mode is 0, 1, 2 or 3
    channel &= 1; // Channel is 0 or 1

    if ((fd = open(channel == 0 ? spiDev0 : spiDev1, O_RDWR)) < 0)
        return wiringPiFailure(WPI_ALMOST, "Unable to open SPI device: %s\n", strerror(errno));

    spiSpeeds [channel] = speed;
    spiFds [channel] = fd;

    // Set SPI parameters.

    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0)
        return wiringPiFailure(WPI_ALMOST, "SPI Mode Change failure: %s\n", strerror(errno));

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spiBPW) < 0)
        return wiringPiFailure(WPI_ALMOST, "SPI BPW Change failure: %s\n", strerror(errno));

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
        return wiringPiFailure(WPI_ALMOST, "SPI Speed Change failure: %s\n", strerror(errno));

    return fd;
}

/*
 * wiringPiSPISetup:
 *	Open the SPI device, and set it up, etc. in the default MODE 0
 *********************************************************************************
 */

int wiringPiSPISetup(int channel, int speed) {
    return wiringPiSPISetupMode(channel, speed, 0);
}


#include <byteswap.h>
#include <stdint.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

int max31855_read(float * value, int spi_fd) {
    uint32_t spiData;
    int temp;
    wiringPiSPIDataRW(spi_fd, (unsigned char *) &spiData, 4);
    spiData = __bswap_32(spiData);
    // Return temp in C * 10
    spiData >>= 18;
    temp = spiData & 0x1FFF; // Bottom 13 bits
    if ((spiData & 0x2000) != 0) // Negative
        temp = -temp;
    *value=(int) ((((double) temp * 25) + 0.5) / 10.0);
    return 1;

}

/*
 * max31855Setup:
 *	Create a new wiringPi device node for an max31855 on the Pi's
 *	SPI interface.
 *********************************************************************************
 */

int max31855_init(int spiChannel) {
    if (wiringPiSPISetup(spiChannel, 5000000) < 0) // 5MHz - prob 4 on the Pi
        return 0;
    return 1;
}