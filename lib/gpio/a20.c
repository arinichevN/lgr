//Allwinner A20 CPU
#define PIO_BASE (0x01C20800) 
#define CCU_BASE    (0x01C20000) 
#define MAP_SIZE (4096*2)
#define MAP_MASK (MAP_SIZE - 1)
#define BLOCK_SIZE  (4*1024)
#define GPIO_NUM  64
#define GPIO_READ(ADDR) *(gpio + (((ADDR) - ((ADDR) & ~MAP_MASK)) >> 2))
#define GPIO_WRITE(VAL, ADDR) *(gpio + (((ADDR) - ((ADDR) & ~MAP_MASK)) >> 2)) = (VAL)

#define PUD_OFF 0
#define PUD_DOWN 2
#define PUD_UP 1

static volatile uint32_t *gpio;

static int physToGpio[GPIO_NUM] = {
    -1, // 0
    -1, -1, //1, 2
    53, -1, //3, 4
    52, -1, //5, 6
    226, 228, //7, 8
    -1, 229, //9, 10
    275, 259, //11, 12
    274, -1, //13, 14
    273, 244, //15, 16
    -1, 245, //17, 18
    268, -1, //19, 20
    269, 272, //21, 22
    267, 266, //23, 24
    -1, 270, //25, 26
    257, 256, //27, 28
    35, -1, //29, 30
    277, 276, //31, 32
    45, -1, //33, 34
    39, 38, //35, 36
    37, 44, //37, 38
    -1, 40, //39, 40
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //41-> 55
    -1, -1, -1, -1, -1, -1, -1, -1 // 56-> 63
};
static uint32_t gpio_data_seek [GPIO_NUM];
static int gpio_data_bank [GPIO_NUM];
static int gpio_data_index [GPIO_NUM];

static uint32_t gpio_data_mode_seek [GPIO_NUM];
static int gpio_data_mode_offset [GPIO_NUM];

static int pin_mask[9][32] = //[BANK]  [INDEX]
{
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PA
    { -1, -1, -1, 3, -1, 5, 6, 7, 8, -1, -1, -1, 12, 13, -1, -1, -1, -1, -1, -1, 20, 21, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PB
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PC
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PD
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PE
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PF
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PG
    { -1, -1, 2, -1, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 20, 21, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PH
    {0, 1, -1, 3, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, -1, 16, 17, 18, 19, 20, 21, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PI
};

void pinWrite(int pin, int value) {
    static uint32_t regval = 0;
    regval = *(gpio + gpio_data_seek[pin]);
    if (value) {
        regval |= (1 << gpio_data_index[pin]);
    } else {
        regval &= ~(1 << gpio_data_index[pin]);
    }
    *(gpio + gpio_data_seek[pin]) = regval;
}

int pinRead(int pin) {
    static uint32_t regval = 0;
    regval = *(gpio + gpio_data_seek[pin]);
    regval = regval >> gpio_data_index[pin];
    regval &= 1;
    return (int) regval;
}

void pinLow(int pin) {
    static uint32_t regval = 0;
    regval = *(gpio + gpio_data_seek[pin]);
    regval &= ~(1 << gpio_data_index[pin]);
    *(gpio + gpio_data_seek[pin]) = regval;
}

void pinHigh(int pin) {
    static uint32_t regval = 0;
    regval = *(gpio + gpio_data_seek[pin]);
    regval |= (1 << gpio_data_index[pin]);
    *(gpio + gpio_data_seek[pin]) = regval;
}

void pinModeIn(int pin) {
    uint32_t regval = 0;
    regval = *(gpio + gpio_data_mode_seek[pin]);
    regval &= ~(7 << gpio_data_mode_offset[pin]);
    *(gpio + gpio_data_mode_seek[pin]) = regval;
}

void pinModeOut(int pin) {
    uint32_t regval = 0;
    regval = *(gpio + gpio_data_mode_seek[pin]);
    regval &= ~(7 << gpio_data_mode_offset[pin]);
    regval |= (1 << gpio_data_mode_offset[pin]);
    *(gpio + gpio_data_mode_seek[pin]) = regval;
}

void pinPUD(int pin, int pud) {
    uint32_t regval = 0;
    int bank = (pin >> 5);
    int index = pin - (bank << 5);
    int sub = index >> 4;
    int sub_index = index - 16 * sub;
    uint32_t phyaddr = PIO_BASE + (bank * 36) + 0x1c + sub * 4;
    regval = GPIO_READ(phyaddr);
    regval &= ~(3 << (sub_index << 1));
    regval |= (pud << (sub_index << 1));
    GPIO_WRITE(regval, phyaddr);
    delayUsIdle(1);
}

void makeGpioDataOffset() {
    int i, pin;
    for (i = 0; i < GPIO_NUM; i++) {
        pin = physToGpio[i];
        if (-1 == pin) {
            gpio_data_seek[i] = 0;
            gpio_data_bank[i] = 0;
            gpio_data_index[i] = 0;
            gpio_data_mode_seek[i] = 0;
            gpio_data_mode_offset[i] = 0;
            continue;
        }
        int bank = pin >> 5;
        int index = pin - (bank << 5);
        int mode_offset = ((index - ((index >> 3) << 3)) << 2);
        uint32_t phyaddr = PIO_BASE + (bank * 36) + 0x10; // +0x10 -> data reg
        uint32_t mode_phyaddr = PIO_BASE + (bank * 36) + ((index >> 3) << 2);
        if (pin_mask[bank][index] != -1) {
            uint32_t mmap_base = (phyaddr & ~MAP_MASK);
            gpio_data_seek[i] = ((phyaddr - mmap_base) >> 2);
            gpio_data_bank[i] = bank;
            gpio_data_index[i] = index;

            mmap_base = (mode_phyaddr & ~MAP_MASK);
            gpio_data_mode_seek[i] = ((mode_phyaddr - mmap_base) >> 2);
            gpio_data_mode_offset[i] = mode_offset;
        } else {
            gpio_data_seek[i] = 0;
            gpio_data_bank[i] = 0;
            gpio_data_index[i] = 0;
            gpio_data_mode_seek[i] = 0;
            gpio_data_mode_offset[i] = 0;
        }
    }
}

int checkPin(int pin) {
    if (pin < 0 || pin >= GPIO_NUM) {
        return 0;
    }
    if (physToGpio[pin] == -1) {
        return 0;
    }
    return 1;
}

int gpioSetup() {
    int fd;
    if ((fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0) {
        fputs("gpioSetup: Unable to open /dev/mem\n", stderr);
        return 0;
    }
    gpio = (volatile uint32_t *) mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CCU_BASE);
    close(fd);
    if (gpio == MAP_FAILED) {
        fputs("gpioSetup: mmap failed\n", stderr);
        return 0;
    }
    
    makeGpioDataOffset();
    return 1;
}

int gpioFree() {
    return 1;
}


