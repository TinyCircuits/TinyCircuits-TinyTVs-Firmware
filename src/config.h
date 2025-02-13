#ifndef CONFIG_H
#define CONFIG_H

#define TV_TYPE_NONE 0  // ints `0` by default, used to error if TV type not set in some parts of the firmware
#define TV_TYPE_2    1
#define TV_TYPE_MINI 2
#define TV_TYPE_KIT  3

// Change this to whatever you are compiling for
#define TV_TYPE TV_TYPE_2

#endif