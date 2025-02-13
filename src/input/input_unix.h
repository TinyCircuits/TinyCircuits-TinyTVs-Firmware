#ifndef INPUT_UNIX_H
#define INPUT_UNIX_H

#include "input_base.h"
#include <stdint.h>

class InputUnix : public InputBase{
    public:
        InputUnix(tv_pins_t *pins);
        ~InputUnix();

        void poll();
    private:
};

#endif