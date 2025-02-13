#ifndef FILES_BASE_H
#define FILES_BASE_H

#include <stdint.h>
#include <stdio.h>

// Platform dependent classes inherit this class
// that defines the format and common logic
class FilesBase{
    public:
        FilesBase(uint16_t max_video_count, uint16_t path_len);
        ~FilesBase();

        // Open next file in list (loops back to
        // start if it reaches the end of the list)
        // Also handles closing previous files first
        virtual void next() = 0;    // `= 0`, pure, must be implemented by derived class

        // Open previous file in list (loops back to
        // end if it reaches the start of the list)
        // Also handles closing previous files first
        virtual void prev() = 0;    // `= 0`, pure, must be implemented by derived class

        // Read up to `size` or `count` bytes into
        // `output` and return number of bytes read
        virtual size_t read(uint8_t *output, size_t size, uint32_t count) = 0; // `= 0`, pure, must be implemented by derived class

        // Move file cursor to `offset` relative to whence:
        //  * `SEEK_SET`: Move to position in file relative to start of file
        //  * `SEEK_CUR`: Move to position in file relative to current position in file
        //  * `SEEK_END`: Move to position in file relative to end of file
        virtual void seek(long offset, int whence) = 0;      // `= 0`, pure, must be implemented by derived class
    private:
        // Open the video at `video_index`
        virtual void open(uint16_t video_index) = 0;    // `= 0`, pure, must be implemented by derived class

        // Close the currently open video file
        virtual void close() = 0;                       // `= 0`, pure, must be implemented by derived class

        uint16_t max_video_count;   // Max number of videos that can be tracked    
        uint16_t path_len;          // Max length of video IDs/LFNs
    protected:
        uint16_t video_index;       // Current video index, or the video we are currently reading from
        uint16_t video_count;       // The actual video count
        char   **video_path_list;   // List of file paths
};

#endif