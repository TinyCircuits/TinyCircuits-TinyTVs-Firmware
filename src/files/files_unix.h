#ifndef FILES_UNIX_H
#define FILES_UNIX_H

#include <stdint.h>

class FilesUnix{
    public:
        FilesUnix();
        ~FilesUnix();

        uint16_t fillVideoNameList(char **video_path_list, uint16_t max_video_count, uint16_t path_name_len);
    private:
        
};

#endif