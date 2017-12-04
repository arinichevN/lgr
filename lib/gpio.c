
#include "gpio.h"

#ifdef CPU_ALLWINNER_A20
#include "gpio/a20.c"
#endif


#ifdef CPU_ALLWINNER_H3
#include "gpio/h3.c"
#endif

#ifdef CPU_ALLWINNER_H3ZERO
#include "gpio/h3p.c"
#endif


#ifdef CPU_CORTEX_A5
#include "gpio/cortex_a5.c"
#endif


//debugging mode (for machine with no GPIO ports)
#ifdef CPU_ANY
#include "gpio/all.c"
#endif