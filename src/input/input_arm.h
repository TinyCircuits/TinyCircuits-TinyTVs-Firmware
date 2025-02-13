#ifndef INPUT_ARM_H
#define INPUT_ARM_H

#include "input_base.h"
#include <stdint.h>

class InputArm : public InputBase{
    public:
        InputArm(tv_pins_t *pins);
        ~InputArm();

        void poll();
    private:
        uint16_t pressed;
};

#endif