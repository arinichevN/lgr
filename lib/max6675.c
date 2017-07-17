
#include "max6675.h"

static void printInt16(uint16_t d) {
    int i;
    for (i = 15; i >= 0; i--) {
        int v = (d >> i) & 1;
        printf("%d", v);
    }
    puts("");
}

int max6675_init(int sclk, int cs, int miso) {
    pinModeOut(cs);
    pinModeOut(sclk);
    pinModeIn(miso);
    pinHigh(cs);
    return 1;
}

int max6675_read(float *result, int sclk, int cs, int miso) {
    uint16_t v;
    pinLow(cs);
    delayUsBusy(1000);
    {
        int i;
        for (i = 15; i >= 0; i--) {
            pinLow(sclk);
            delayUsBusy(1000);
            if (pinRead(miso)) {
                v |= (1 << i);
            }
            pinHigh(sclk);
            delayUsBusy(1000);
        }
    }
    pinHigh(cs);
#ifdef MODE_DEBUG
    printInt16(v);
#endif
    if (v & 0x4) {
#ifdef MODE_DEBUG
        fputs("max6675_read: thermocouple input is open\n", stderr);
#endif
        return 0;
    }
    v >>= 3;
    *result = v * 0.25;
    return 1;
}

int max6675_so_init(int sclk, int cs, int *miso, int miso_length) {
    pinModeOut(cs);
    pinModeOut(sclk);
    int i;
    for (i = 0; i < miso_length; i++) {
        pinModeIn(miso[i]);
    }
    pinHigh(cs);
    return 1;
}

//so pin is individual for each chip, cs and sclk are common.

void max6675_so_read(MAX6675DataList *data, int sclk, int cs) {
    uint16_t v[data->length];
    pinLow(cs);
    delayUsBusy(1000);

    int i, j;
    for (i = 15; i >= 0; i--) {
        pinLow(sclk);
        delayUsBusy(1000);
        for (j = 0; j < data->length; j++) {
            if (pinRead(data->item[j].miso)) {
                v[j] |= (1 << i);
            }
        }
        pinHigh(sclk);
        delayUsBusy(1000);
    }

    pinHigh(cs);
    for (j = 0; j < data->length; j++) {
#ifdef MODE_DEBUG
        printInt16(v[j]);
#endif
        data->item[j].state = 1;
        if (v[j] & 0x4) {
#ifdef MODE_DEBUG
            fprintf(stderr, "max6675_so_read: the thermocouple input is open where miso pin is %d\n", data->item[j].miso);
#endif
            data->item[j].state = 0;
        }
        v[j] >>= 3;
        data->item[j].value = v[j] * 0.25;
    }
}

