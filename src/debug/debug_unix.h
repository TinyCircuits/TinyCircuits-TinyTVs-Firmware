#ifndef DEBUG_UNIX_H
#define DEBUG_UNIX_H

#ifndef debug_print
    #include <iostream>
    #define debug_println(x) std::cout << x << std::endl;
#endif

void debug_init();

#endif