#include "files_base.h"
#include "../debug/debug.h"

FilesBase::FilesBase(uint16_t max_video_count, uint16_t path_len){
    debug_println("Files Base");

    // Store these
    max_video_count = max_video_count;
    path_len = path_len;

    // Allocate memory for list of file name IDs (SD cards have LFN names)
    video_path_list = (char **)malloc(max_video_count * sizeof(char *));
    for(uint16_t i=0; i<max_video_count; i++){
        video_path_list[i] = (char *)malloc(path_len * sizeof(char));
    }
}


FilesBase::~FilesBase(){

}