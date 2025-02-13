#ifndef FILES_BASE_H
#define FILES_BASE_H

#include <stdint.h>

// Platform dependent classes inherit this class
// that defines the format and common logic
class FilesBase{
    public:
        FilesBase(uint16_t max_video_count, uint16_t path_len);
        ~FilesBase();
    private:
        uint16_t max_video_count;   // Max number of videos that can be tracked    
        uint16_t path_len;          // Max length of video IDs/LFNs
    protected:
        uint16_t video_count;       // The actual video count
        char   **video_path_list;   // List of file paths
};

#endif