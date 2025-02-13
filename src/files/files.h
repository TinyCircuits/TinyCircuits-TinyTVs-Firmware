#ifndef FILES_H
#define FILES_H

#include <stdint.h>

// Figure out which files base to inherit from
#if defined(__unix__)
    #include "files_unix.h"
    #define Files FilesUnix
#elif defined(__arm__)
    #include "files_arm.h"
    #define Files FilesArm
#endif


#endif