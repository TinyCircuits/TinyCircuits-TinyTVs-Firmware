#include "files_base.h"
#include "../debug/debug.h"

FilesBase::FilesBase(uint16_t max_video_count, uint16_t path_len){
    debug_println("Files Base");

    // Store these
    this->max_video_count = max_video_count;
    this->path_len = path_len;

    // Allocate memory for list of file name IDs (SD cards have LFN names)
    video_path_list = (char **)malloc(max_video_count * sizeof(char *));
    for(uint16_t i=0; i<max_video_count; i++){
        video_path_list[i] = (char *)malloc(path_len * sizeof(char));
    }

    // Set to defaults
    this->video_count = 0;
    this->video_index = 0;
}


FilesBase::~FilesBase(){
    // Free the allocated memory for file names
    for (uint16_t i = 0; i < max_video_count; i++){
        free(video_path_list[i]);
    }

    free(video_path_list);
}


void FilesBase::next(){
    if(video_index >= video_count-1){
        video_index = 0;
    }else{
        video_index++;
    }
}


void FilesBase::prev(){
    if(video_index <= 0){
        video_index = video_count-1;
    }else{
        video_index--;
    }
}