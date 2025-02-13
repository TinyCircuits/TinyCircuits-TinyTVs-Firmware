#include "input_arm.h"
#include "../debug/debug.h"

InputArm::InputArm(tv_pins_t *pins) : InputBase(pins){
    debug_println("Input Arm");
}


InputArm::~InputArm(){

}


void InputArm::poll(){

}


bool InputArm::is_power_pressed(){
    return false;
}


bool InputArm::is_next_channel_pressed(){
    return false;
}


bool InputArm::is_prev_channel_pressed(){
    return false;
}


bool InputArm::is_vol_up_pressed(){
    return false;
}


bool InputArm::is_vol_down_pressed(){
    return false;
}