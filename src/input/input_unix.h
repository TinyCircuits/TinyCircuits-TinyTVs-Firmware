#ifndef INPUT_UNIX_H
#define INPUT_UNIX_H

#include <stdint.h>

typedef struct tv2_input_t{
    uint8_t lencoder_pin_a;
    uint8_t lencoder_pin_b;
    uint8_t rencoder_pin_a;
    uint8_t rencoder_pin_b;
}tv2_input_t;

typedef struct tvmini_input_t{
    uint8_t lbtn_pin;
    uint8_t rbtn_pin;
}tvmini_input_t;

typedef struct tvkit_input_t{
    uint8_t lside_top_btn_pin;
    uint8_t lside_bot_btn_pin;
    uint8_t rside_top_btn_pin;
    uint8_t rside_bot_btn_pin
}tvkit_input_t;

typedef struct input_t{
    uint8_t tv_type;
    uint8_t pwr_btn_pin;
    uint8_t pwr_btn_off_pin;
    uint8_t ir_pin;

    union{
        struct tv2_input_t    tv2;
        struct tvmini_input_t tvmini;
        struct tvkit_input_t  tvkit;
    }base;
}input_t;


class InputUnix{
    public:
        InputUnix(uint8_t pwr_btn_pin, uint8_t pwr_btn_off_pin, uint8_t ir_pin, uint8_t lencoder_pin_a, uint8_t lencoder_pin_b, uint8_t rencoder_pin_a, uint8_t rencoder_pin_b);                // TV2
        InputUnix(uint8_t pwr_btn_pin, uint8_t pwr_btn_off_pin, uint8_t ir_pin, uint8_t lbtn_pin, uint8_t rbtn_pin);                                                                            // TVMini
        InputUnix(uint8_t pwr_btn_pin, uint8_t pwr_btn_off_pin, uint8_t ir_pin, uint8_t lside_top_btn_pin, uint8_t lside_bot_btn_pin, uint8_t rside_top_btn_pin, uint8_t rside_bot_btn_pin);    // TVKit
        ~InputUnix();

        void poll();

        bool is_power_pressed();
        bool is_next_channel_pressed();
        bool is_prev_channel_pressed();
        bool is_vol_up_pressed();
        bool is_vol_down_pressed();
    private:
        uint16_t pressed;
        input_t input;
};

#endif