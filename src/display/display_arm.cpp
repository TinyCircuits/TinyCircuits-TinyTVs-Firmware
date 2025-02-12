#include "display_arm.h"
#include "../debug/debug.h"

DisplayArm::DisplayArm(uint16_t width, uint16_t height){
    debug_println("Display Arm");

    // Store these
    width = width;
    height = height;
}


DisplayArm::~DisplayArm(){

}


void DisplayArm::write(uint16_t *buffer, uint32_t buffer_len){

}


void DisplayArm::update(){

}