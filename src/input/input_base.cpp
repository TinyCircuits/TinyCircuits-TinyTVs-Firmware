#include "input_base.h"
#include "../debug/debug.h"

InputBase::InputBase(tv_pins_t *pins){
    debug_println("Input Base");

    // Store this
    this->pins = pins;

    // Set these
    this->last_pressed = 0;
    this->pressed = 0;
}

InputBase::~InputBase(){

}


bool InputBase::is_power_pressed(bool check_just_pressed){
    if(check_just_pressed){
        return (INPUT_FUNC_CODE_PWR & (pressed & ~last_pressed));
    }else{
        return (INPUT_FUNC_CODE_PWR & pressed);
    }
}


bool InputBase::is_next_channel_pressed(bool check_just_pressed){
    if(check_just_pressed){
        return (INPUT_FUNC_CODE_NEXT_CHAN & (pressed & ~last_pressed));
    }else{
        return (INPUT_FUNC_CODE_NEXT_CHAN & pressed);
    }
}


bool InputBase::is_prev_channel_pressed(bool check_just_pressed){
    if(check_just_pressed){
        return (INPUT_FUNC_CODE_PREV_CHAN & (pressed & ~last_pressed));
    }else{
        return (INPUT_FUNC_CODE_PREV_CHAN & pressed);
    }
}


bool InputBase::is_vol_up_pressed(bool check_just_pressed){
    if(check_just_pressed){
        return (INPUT_FUNC_CODE_VOL_UP & (pressed & ~last_pressed));
    }else{
        return (INPUT_FUNC_CODE_VOL_UP & pressed);
    }
}


bool InputBase::is_vol_down_pressed(bool check_just_pressed){
    if(check_just_pressed){
        return (INPUT_FUNC_CODE_VOL_DOWN & (pressed & ~last_pressed));
    }else{
        return (INPUT_FUNC_CODE_VOL_DOWN & pressed);
    }
}