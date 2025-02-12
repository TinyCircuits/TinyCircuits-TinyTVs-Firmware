#ifndef FILES_H
#define FILES_H

#include <stdint.h>

// Figure out which files base to inherit from
#if defined(__unix__)
    #include "files_unix.h"
    #define FilesBase FilesUnix
#elif defined(__arm__)
    #include "files_arm.h"
    #define FilesBase FilesArm
#endif


class Files : public FilesBase{
    public:
        Files(uint16_t max_video_count, uint16_t path_name_len);
        ~Files();
    private:
        uint16_t video_count;       // The actual video count
        uint16_t max_video_count;   // Max number of videos that can be tracked    
        uint16_t path_len;          // Max length of video IDs/LFNs
        char   **video_path_list;   // List of file paths
};

#endif