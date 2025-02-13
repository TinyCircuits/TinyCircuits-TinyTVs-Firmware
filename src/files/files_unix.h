#ifndef FILES_UNIX_H
#define FILES_UNIX_H

#include "files_base.h"
#include <stdint.h>

class FilesUnix : public FilesBase{
    public:
        FilesUnix(uint16_t max_video_count, uint16_t path_name_len);
        ~FilesUnix();
    private:
        
};

#endif