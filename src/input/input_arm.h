#ifndef INPUT_ARM_H
#define INPUT_ARM_H

#include "input_base.h"
#include <stdint.h>

class InputArm : public InputBase{
    public:
        InputArm(tv_pins_t *pins);
        ~InputArm();

        void poll();

        bool is_power_pressed();
        bool is_next_channel_pressed();
        bool is_prev_channel_pressed();
        bool is_vol_up_pressed();
        bool is_vol_down_pressed();
    private:
        uint16_t pressed;
};

#endif