#ifndef FILES_ARM_H
#define FILES_ARM_H

#include <stdint.h>

class FilesArm{
    public:
        FilesArm();
        ~FilesArm();

        uint16_t fillVideoNameList(char **video_path_list, uint16_t max_video_count, uint16_t path_name_len);
    private:
        
};

#endif