#ifndef DISPLAY_H
#define DISPLAY_H

#if defined(__unix__)
    #include "display_unix.h"
    #define Display DisplayUnix
#elif defined(__arm__)
    #include "display_arm.h"
    #define Display DisplayArm
#endif

#endif