#ifndef FILES_ARM_H
#define FILES_ARM_H

#include "files_base.h"
#include <stdint.h>


class FilesArm : public FilesBase{
    public:
        FilesArm(uint16_t max_video_count, uint16_t path_len);
        ~FilesArm();

        void next();
        void prev();
        void read(uint8_t *output, size_t size, uint32_t count);
        void seek(long offset, int whence);
    private:
        
};

#endif