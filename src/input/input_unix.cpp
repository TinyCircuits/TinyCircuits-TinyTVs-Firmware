#include "input_unix.h"
#include "input_codes.h"
#include "../debug/debug.h"
#include "../config.h"
#include <SDL2/SDL.h>


InputUnix::InputUnix(uint8_t pwr_btn_pin, uint8_t pwr_btn_off_pin, uint8_t ir_pin, uint8_t lencoder_pin_a, uint8_t lencoder_pin_b, uint8_t rencoder_pin_a, uint8_t rencoder_pin_b){
    input.tv_type         = TV_TYPE_2;
    input.pwr_btn_pin     = pwr_btn_pin;
    input.pwr_btn_off_pin = pwr_btn_off_pin;
    input.ir_pin          = ir_pin;

    input.base.tv2.lencoder_pin_a = lencoder_pin_a;
    input.base.tv2.lencoder_pin_b = lencoder_pin_b;
    input.base.tv2.rencoder_pin_a = rencoder_pin_a;
    input.base.tv2.rencoder_pin_b = rencoder_pin_b;
}


InputUnix::InputUnix(uint8_t pwr_btn_pin, uint8_t pwr_btn_off_pin, uint8_t ir_pin, uint8_t lbtn_pin, uint8_t rbtn_pin){
    input.tv_type         = TV_TYPE_MINI;
    input.pwr_btn_pin     = pwr_btn_pin;
    input.pwr_btn_off_pin = pwr_btn_off_pin;
    input.ir_pin          = ir_pin;

    input.base.tvmini.lbtn_pin = lbtn_pin;
    input.base.tvmini.rbtn_pin = rbtn_pin;
}


InputUnix::InputUnix(uint8_t pwr_btn_pin, uint8_t pwr_btn_off_pin, uint8_t ir_pin, uint8_t lside_top_btn_pin, uint8_t lside_bot_btn_pin, uint8_t rside_top_btn_pin, uint8_t rside_bot_btn_pin){
    input.tv_type         = TV_TYPE_KIT;
    input.pwr_btn_pin     = pwr_btn_pin;
    input.pwr_btn_off_pin = pwr_btn_off_pin;
    input.ir_pin          = ir_pin;

    input.base.tvkit.lside_top_btn_pin = lside_top_btn_pin;
    input.base.tvkit.lside_bot_btn_pin = lside_bot_btn_pin;
    input.base.tvkit.rside_top_btn_pin = rside_top_btn_pin;
    input.base.tvkit.rside_bot_btn_pin = rside_bot_btn_pin;
}


InputUnix::~InputUnix(){

}


void InputUnix::poll(){

}


bool InputUnix::is_power_pressed(){

}


bool InputUnix::is_next_channel_pressed(){

}


bool InputUnix::is_prev_channel_pressed(){

}


bool InputUnix::is_vol_up_pressed(){

}


bool InputUnix::is_vol_down_pressed(){

}