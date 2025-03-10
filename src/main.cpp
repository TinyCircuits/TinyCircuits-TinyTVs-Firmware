#if defined(__unix__)


#include "debug/debug.h"
#include "files/files.h"
#include "display/display.h"
#include "tinytv.h"
#include "input/input.h"
#include "demuxer/demuxer.h"
#include "config.h"

// https://wiki.libsdl.org/SDL2/SDLKeycodeLookup
tv_pins_t pins = {
    .tv_type         = TV_TYPE_2,
    
    .pwr_btn_pin     = 111, // o
    .ir_pin          = 105, // i

    .unique = {
        .tv2_pins = {
            .lencoder_pin_a = 113,  // q
            .lencoder_pin_b = 101,  // e

            .rencoder_pin_a = 97,   // a
            .rencoder_pin_b = 100,  // d
        }
    },
};


Files files(100, 150);
Display display(208, 136);
TinyTV tv(&files, &display);
Input input(&pins);
Demuxer demuxer(&files);


int main(int argc, char *argv[]){
    debug_init();

    demuxer.begin();

    while(true){
        input.poll();

        if(input.is_power_pressed()){
            break;
        }

        if(input.is_next_channel_pressed()){
            files.next();
            demuxer.begin();
        }else if(input.is_prev_channel_pressed()){
            files.prev();
            demuxer.begin();
        }

        if(input.is_vol_up_pressed()){
            demuxer.get_next_video_data(NULL, 0);
        }else if(input.is_vol_down_pressed()){
            
        }
    }

    // tv.processVideo();
    return 0;
}


#endif