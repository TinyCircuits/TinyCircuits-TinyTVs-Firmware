#include "input_unix.h"
#include "../debug/debug.h"
#include "../config.h"
#include <SDL2/SDL.h>


InputUnix::InputUnix(tv_pins_t *pins) : InputBase(pins){
    debug_println("Input Unix");
}


InputUnix::~InputUnix(){

}


void InputUnix::poll(){

}


bool InputUnix::is_power_pressed(){
    return false;
}


bool InputUnix::is_next_channel_pressed(){
    return false;
}


bool InputUnix::is_prev_channel_pressed(){
    return false;
}


bool InputUnix::is_vol_up_pressed(){
    return false;
}


bool InputUnix::is_vol_down_pressed(){
    return false;
}