#ifndef DEBUG_ARM_H
#define DEBUG_ARM_H

#ifndef debug_print
    extern Adafruit_USBD_CDC cdc;
    #define debug_println(x) cdc.println(x);
#endif

void debug_init();

#endif