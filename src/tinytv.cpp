#include "tinytv.h"
#include "debug/debug.h"

TinyTV::TinyTV(Files *files, Display *display){
    debug_println("TinyTV");

    files = files;
    display = display;
}


TinyTV::~TinyTV(){
    
}


TVErrorCode TinyTV::processVideo(){
    
    return TVErrorCode::TV_OK;
}