#ifndef INPUT_UNIX_H
#define INPUT_UNIX_H

#include "input_base.h"
#include <stdint.h>

class InputUnix : public InputBase{
    public:
        InputUnix(tv_pins_t *pins);
        ~InputUnix();

        void poll();

        bool is_power_pressed();
        bool is_next_channel_pressed();
        bool is_prev_channel_pressed();
        bool is_vol_up_pressed();
        bool is_vol_down_pressed();
    private:
};

#endif