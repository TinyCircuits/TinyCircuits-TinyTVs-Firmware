#if defined(__arm__)

#include "files_arm.h"
#include "../debug/debug.h"


FilesArm::FilesArm(uint16_t max_video_count, uint16_t path_len) : FilesBase(max_video_count, path_len){
    debug_println("Files Arm");
}


FilesArm::~FilesArm(){
    
}


#endif