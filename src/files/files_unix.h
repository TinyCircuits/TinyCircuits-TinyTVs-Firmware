#ifndef FILES_UNIX_H
#define FILES_UNIX_H

#include "files_base.h"
#include <stdint.h>


class FilesUnix : public FilesBase{
    public:
        FilesUnix(uint16_t max_video_count, uint16_t path_name_len);
        ~FilesUnix();

        void next() override;
        void prev() override;
        ssize_t video_read(uint8_t *output, uint32_t count) override;
        off_t video_seek(long offset, int whence) override;
        off_t video_size() override;
    private:
        void open_video(uint16_t video_index) override;
        void close_video() override;

        // Linux file descriptor
        int open_file;
};

#endif