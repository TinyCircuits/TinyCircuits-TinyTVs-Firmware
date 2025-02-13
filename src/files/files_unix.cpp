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

    // Set to default
    open_file = NULL;

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

    open(video_index);
}


FilesUnix::~FilesUnix(){
    close();
}


void FilesUnix::next(){
    // Close current video
    close();

    // Handles `video_index` increment and loop
    FilesBase::next();

    // Open the next video
    open(video_index);
}


void FilesUnix::prev(){
    // Close current video
    close();

    // Handles `video_index` decrement and loop
    FilesBase::prev();

    // Open the prev video
    open(video_index);
}


size_t FilesUnix::read(uint8_t *output, size_t size, uint32_t count){
    return fread(output, size, count, open_file);
}


void FilesUnix::seek(long offset, int whence){
    fseek(open_file, offset, whence);
}


void FilesUnix::open(uint16_t video_index){
    open_file = fopen(video_path_list[video_index], "r");

    if(open_file == NULL){
        debug_println("ERROR: Could not open file!");
    }

    debug_println("SUCCESS: Opened file!");
    debug_println(video_path_list[video_index]);
}


void FilesUnix::close(){
    if(open_file){
        fclose(open_file);
        open_file = NULL;

        debug_println("SUCCESS: Closed file!");
        debug_println(video_path_list[video_index]);
    }
}


#endif