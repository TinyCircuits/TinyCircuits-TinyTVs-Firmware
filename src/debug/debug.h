#ifndef DEBUG_H
#define DEBUG_H

#if defined(__unix__)
    #include "debug_unix.h"
#elif defined(__arm__)
    #include "debug_arm.h"
#endif

#endif