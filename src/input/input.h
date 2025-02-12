#ifndef INPUT_H
#define INPUT_H

#if defined(__unix__)
    #include "input_unix.h"
    #define Input InputUnix
#elif defined(__arm__)
    #include "input_arm.h"
    #define Input InputArm
#endif

#endif