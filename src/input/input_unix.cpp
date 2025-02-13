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
    static SDL_Event event;

    while(SDL_PollEvent(&event)){
        int32_t code = event.key.keysym.sym;

        if(event.type == SDL_KEYDOWN){
            if(code == pins->pwr_btn_pin){
                pressed |= INPUT_FUNC_CODE_PWR;
            }
        }else if(event.type == SDL_KEYUP){
            if(code == pins->pwr_btn_pin){
                pressed &= ~INPUT_FUNC_CODE_PWR;
            }
        }
    }
}