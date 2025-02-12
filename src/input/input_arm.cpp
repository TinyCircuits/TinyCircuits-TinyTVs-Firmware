#include "input_arm.h"
#include "../debug/debug.h"

InputArm::InputArm(){
    debug_println("Input Arm");
}


InputArm::~InputArm(){

}


void InputArm::poll(){

}


bool InputArm::is_power_pressed(){
    return false;
}


bool InputArm::lknob_went_left(){
    return false;
}


bool InputArm::lknob_went_right(){
    return false;
}


bool InputArm::rknob_went_left(){
    return false;
}


bool InputArm::rknob_went_right(){
    return false;
}