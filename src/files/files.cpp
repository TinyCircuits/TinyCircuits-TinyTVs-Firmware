#include "files.h"
#include "../debug/debug.h"

#include <stdlib.h>

Files::Files(uint16_t max_video_count, uint16_t path_name_len){
    debug_println("Files");

    // Store these
    max_video_count = max_video_count;
    path_name_len = path_name_len;

    // Allocate memory for list of file name IDs (SD cards have LFN names)
    video_path_list = (char **)malloc(max_video_count * sizeof(char *));
    for(uint16_t i=0; i<max_video_count; i++){
        video_path_list[i] = (char *)malloc(path_name_len * sizeof(char));
    }

    // Fill list of names for quick browsing (cache),
    // and keep track of the number of videos actually
    // found
    video_count = fillVideoNameList(video_path_list, max_video_count, path_name_len);
}


Files::~Files(){
    // Free the allocated memory for file names
    for (uint16_t i = 0; i < max_video_count; i++){
        free(video_path_list[i]);
    }

    free(video_path_list);
}