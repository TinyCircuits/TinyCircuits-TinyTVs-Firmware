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


ssize_t FilesArm::video_read(uint8_t *output, size_t size){
    return 0;
}


off_t FilesArm::video_seek(long offset, int whence){
    return 0;
}


off_t FilesArm::video_size(){
    return 0;
}


void FilesArm::open_video(uint16_t video_index){

}


void FilesArm::close_video(){

}


#endif