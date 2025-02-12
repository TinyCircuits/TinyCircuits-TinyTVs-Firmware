#if defined(__arm__)

#include "tinytv.h"
#include "debug/debug.h"
#include "files/files.h"

Files files(100, 150);
TinyTV tv;

void setup(){
    debug_init();

    tv.processVideo();
}


void loop(){

}


#endif