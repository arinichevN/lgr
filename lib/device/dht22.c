#include "dht22.h"

#define DHT22_MAXTIMINGS 85
#define DHT22_MAX_VAL 83

int dht22_read(int pin, float *t, float *h) {
    //sending request
    pinPUD(pin, PUD_OFF);
    pinModeOut(pin);
    pinHigh(pin);
    delayUsBusy(250);
    pinLow(pin);
    delayUsBusy(500);
    pinPUD(pin, PUD_UP);
    delayUsBusy(15);

    pinModeIn(pin);

    //reading response from chip
    int i;
    int laststate = HIGH;
    int arr[DHT22_MAXTIMINGS]; //we will write signal duration here
    memset(arr, 0, sizeof arr);

    for (i = 0; i < DHT22_MAXTIMINGS; i++) {
        int c = 0;
        while (pinRead(pin) == laststate) {
            c++;
            delayUsBusy(1);
            if (c >= DHT22_MAX_VAL) {
                break;
            }
        }
        laststate = pinRead(pin);
        arr[i] = c;
    }

    //dealing with response from chip
    int j = 0; //bit counter
    uint8_t data[5] = {0, 0, 0, 0, 0};
    for (i = 0; i < DHT22_MAXTIMINGS; i++) {
        if (arr[i] > DHT22_MAX_VAL) {//too long signal found
            break;
        }
        if (i < 3) {//skip first 3 signals (response signal)
            continue;
        }
        if (i % 2 == 0) {//dealing with data signal
            if (j >= 40) {//we have 8*5 bits of data
                break;
            }
            data[j / 8] <<= 1; //put 0 bit
            if (arr[i] > arr[i - 1]) {//if (high signal duration > low signal duration) then 1
                data[j / 8] |= 1;
            }
            j++;
        }
    }

#ifdef MODE_DEBUG
    printf("dht22_read: data: %.2hhx %.2hhx %.2hhx %.2hhx %.2hhx, j=%d\n", data[0], data[1], data[2], data[3], data[4], j);
    for (i = 0; i < DHT22_MAXTIMINGS; i++) {
        printf("%d ", arr[i]);
    }
    puts("");
#endif
    if (j < 40) {
        printde("j=%d but j>=40 expected where pin=%d\n", j, pin);
        return 0;
    }
    if ((data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        *h = (float) data[0] * 256 + (float) data[1];
        *h /= 10;
        *t = (float) (data[2] & 0x7F)* 256 + (float) data[3];
        *t /= 10.0;
        if ((data[2] & 0x80) != 0) {
            *t *= -1;
        }
        return 1;
    }
    printde("bad crc where pin=%d\n", pin);
    return 0;

}