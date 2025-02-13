#if defined(__unix__)

#include "files_unix.h"
#include "../debug/debug.h"

#include <string>
#include <cstring>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

FilesUnix::FilesUnix(uint16_t max_video_count, uint16_t path_name_len) : FilesBase(max_video_count, path_name_len){
    debug_println("Files Unix");

    video_count = 0;
    std::string video_path = "../../videos";

    for (const auto & entry : fs::directory_iterator(video_path)){
        // Needs to be regular file, otherwise, skip
        if(!entry.is_regular_file()){
            continue;
        }

        // Get just the file path of each entry
        std::string path = entry.path();

        // Do not store file paths that go out of bounds
        if(path.length() > path_name_len){
            continue;
        }

        // Conforms to path requirements
        debug_println(path);
        strncpy(video_path_list[video_count], path.c_str(), path_name_len);
        video_count++;
    }
}


FilesUnix::~FilesUnix(){
    
}


#endif