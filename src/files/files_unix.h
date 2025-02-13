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
        size_t read(uint8_t *output, size_t size, uint32_t count);
        void seek(long offset, int whence);
    private:
        void open(uint16_t video_index);
        void close();

        FILE *open_file;
};

#endif