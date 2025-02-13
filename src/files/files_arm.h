#ifndef FILES_ARM_H
#define FILES_ARM_H

#include "files_base.h"
#include <stdint.h>

class FilesArm : public FilesBase{
    public:
        FilesArm(uint16_t max_video_count, uint16_t path_len);
        ~FilesArm();
    private:
        
};

#endif