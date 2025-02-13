#ifndef INPUT_ARM_H
#define INPUT_ARM_H

#include <stdint.h>

class InputArm{
    public:
        InputArm(uint8_t lencoder_pin_a, uint8_t lencoder_pin_b, uint8_t rencoder_pin_a, uint8_t rencoder_pin_b, uint8_t pwr_btn_pin, uint8_t pwr_btn_off_pin, uint8_t ir_pin);                // TV2
        InputArm(uint8_t lbtn_pin, uint8_t rbtn_pin, uint8_t pwr_btn_pin, uint8_t pwr_btn_off_pin, uint8_t ir_pin);                                                                            // TVMini
        InputArm(uint8_t lside_top_btn_pin, uint8_t lside_bot_btn_pin, uint8_t rside_top_btn_pin, uint8_t rside_bot_btn_pin, uint8_t pwr_btn_pin, uint8_t pwr_btn_off_pin, uint8_t ir_pin);    // TVKit
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