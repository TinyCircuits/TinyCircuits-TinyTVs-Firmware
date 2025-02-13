#include "input_arm.h"
#include "../debug/debug.h"

InputArm::InputArm(tv_pins_t *pins) : InputBase(pins){
    debug_println("Input Arm");
}


InputArm::~InputArm(){

}


void InputArm::poll(){

}