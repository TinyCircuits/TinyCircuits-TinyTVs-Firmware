#include "display_base.h"
#include "../debug/debug.h"


DisplayBase::DisplayBase(uint16_t width, uint16_t height){
    debug_println("Display Base");

    // Store these
    this->width = width;
    this->height = height;
}


DisplayBase::~DisplayBase(){

}