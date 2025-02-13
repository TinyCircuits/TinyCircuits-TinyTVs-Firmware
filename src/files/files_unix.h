#ifndef FILES_UNIX_H
#define FILES_UNIX_H

#include "files_base.h"
#include <stdint.h>


class FilesUnix : public FilesBase{
    public:
        FilesUnix(uint16_t max_video_count, uint16_t path_name_len);
        ~FilesUnix();

        void next();
        void prev();
        ssize_t read_video(uint8_t *output, uint32_t count);
        off_t seek_video(long offset, int whence);
    private:
        void open_video(uint16_t video_index);
        void close_video();

        // Linux file descriptor
        int open_file;
};

#endif