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

    // Store this before updating pressed so
    // we can check if ever just pressed
    last_pressed = pressed;

    while(SDL_PollEvent(&event)){
        int32_t code = event.key.keysym.sym;

        if(event.type == SDL_KEYDOWN){
            if(code == pins->pwr_btn_pin){
                pressed |= INPUT_FUNC_CODE_PWR;
            }else if(code == pins->unique.tv2_pins.lencoder_pin_a){
                pressed |= INPUT_FUNC_CODE_VOL_DOWN;
            }else if(code == pins->unique.tv2_pins.lencoder_pin_b){
                pressed |= INPUT_FUNC_CODE_VOL_UP;
            }else if(code == pins->unique.tv2_pins.rencoder_pin_a){
                pressed |= INPUT_FUNC_CODE_PREV_CHAN;
            }else if(code == pins->unique.tv2_pins.rencoder_pin_b){
                pressed |= INPUT_FUNC_CODE_NEXT_CHAN;
            }
        }else if(event.type == SDL_KEYUP){
            if(code == pins->pwr_btn_pin){
                pressed &= ~INPUT_FUNC_CODE_PWR;
            }else if(code == pins->unique.tv2_pins.lencoder_pin_a){
                pressed &= ~INPUT_FUNC_CODE_VOL_DOWN;
            }else if(code == pins->unique.tv2_pins.lencoder_pin_b){
                pressed &= ~INPUT_FUNC_CODE_VOL_UP;
            }else if(code == pins->unique.tv2_pins.rencoder_pin_a){
                pressed &= ~INPUT_FUNC_CODE_PREV_CHAN;
            }else if(code == pins->unique.tv2_pins.rencoder_pin_b){
                pressed &= ~INPUT_FUNC_CODE_NEXT_CHAN;
            }
        }
    }
}