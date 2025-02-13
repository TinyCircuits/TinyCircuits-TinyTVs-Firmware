#ifndef FILES_ARM_H
#define FILES_ARM_H

#include "files_base.h"
#include <stdint.h>


class FilesArm : public FilesBase{
    public:
        FilesArm(uint16_t max_video_count, uint16_t path_len);
        ~FilesArm();

        void next() override;
        void prev() override;
        ssize_t read_video(uint8_t *output, uint32_t count) override;
        off_t seek_video(long offset, int whence) override;
    private:
        virtual void open_video(uint16_t video_index) override;
        virtual void close_video() override;
};

#endif