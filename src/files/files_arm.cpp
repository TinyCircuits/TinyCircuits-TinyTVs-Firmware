#if defined(__arm__)

#include "files_arm.h"
#include "../debug/debug.h"


FilesArm::FilesArm(){
    debug_println("Files Arm");
}


FilesArm::~FilesArm(){
    
}


uint16_t FilesArm::fillVideoNameList(char **video_path_list, uint16_t max_video_count, uint16_t path_name_len){
    return 0;
}


#endif