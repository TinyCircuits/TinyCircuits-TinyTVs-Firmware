#if defined(__arm__)

#include "files_arm.h"
#include "../debug/debug.h"


FilesArm::FilesArm(uint16_t max_video_count, uint16_t path_len) : FilesBase(max_video_count, path_len){
    debug_println("Files Arm");
}


FilesArm::~FilesArm(){
    
}


void FilesArm::next(){

}


void FilesArm::prev(){

}


void FilesArm::read(uint8_t *output, size_t size, uint32_t count){

}


void FilesArm::seek(long offset, int whence){

}


#endif