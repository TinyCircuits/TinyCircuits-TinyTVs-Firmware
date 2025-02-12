#include "input_unix.h"
#include "../debug/debug.h"

InputUnix::InputUnix(){
    debug_println("Input Unix");
}


InputUnix::~InputUnix(){

}


void InputUnix::poll(){

}


bool InputUnix::is_power_pressed(){
    return false;
}


bool InputUnix::lknob_went_left(){
    return false;
}


bool InputUnix::lknob_went_right(){
    return false;
}


bool InputUnix::rknob_went_left(){
    return false;
}


bool InputUnix::rknob_went_right(){
    return false;
}