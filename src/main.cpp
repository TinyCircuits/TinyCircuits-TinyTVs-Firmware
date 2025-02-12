#if defined(__unix__)


#include "debug/debug.h"
#include "files/files.h"
#include "display/display.h"
#include "tinytv.h"
#include "input/input.h"

Files files(100, 150);
Display display(128, 128);
TinyTV tv(&files, &display);
Input input;


int main(int argc, char *argv[]){
    debug_init();

    while(true){
        input.poll();

        if(input.is_power_pressed()){
            break;
        }
    }

    // tv.processVideo();
    return 0;
}


#endif