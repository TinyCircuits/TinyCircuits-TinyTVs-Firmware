#if defined(__unix__)

#include "files_unix.h"
#include "../debug/debug.h"

#include <string>
#include <cstring>
#include <iostream>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
namespace fs = std::filesystem;

FilesUnix::FilesUnix(uint16_t max_video_count, uint16_t path_name_len) : FilesBase(max_video_count, path_name_len){
    debug_println("Files Unix");

    // Set to default
    open_file = -1;

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

    open_video(video_index);
}


FilesUnix::~FilesUnix(){
    close_video();
}


void FilesUnix::next(){
    // Close current video
    close_video();

    // Handles `video_index` increment and loop
    FilesBase::next();

    // Open the next video
    open_video(video_index);
}


void FilesUnix::prev(){
    // Close current video
    close_video();

    // Handles `video_index` decrement and loop
    FilesBase::prev();

    // Open the prev video
    open_video(video_index);
}


ssize_t FilesUnix::read_video(uint8_t *output, uint32_t count){
    return read(open_file, output, count);
}


off_t FilesUnix::seek_video(long offset, int whence){
    return lseek(open_file, offset, whence);
}


void FilesUnix::open_video(uint16_t video_index){
    // open_file = fopen(video_path_list[video_index], "r");
    open_file = open(video_path_list[video_index], O_RDONLY);

    if(open_file == -1){
        debug_println("ERROR: Could not open file!");
    }

    debug_println("SUCCESS: Opened file!");
    debug_println(video_path_list[video_index]);
}


void FilesUnix::close_video(){
    if(open_file != -1){
        close(open_file);
        open_file = -1;

        debug_println("SUCCESS: Closed file!");
        debug_println(video_path_list[video_index]);
    }
}


#endif