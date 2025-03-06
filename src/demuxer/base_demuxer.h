#ifndef BASE_DEMUXER_H
#define BASE_DEMUXER_H

#include "../files/files.h"
#include <stdint.h>
#include <stdio.h>

enum ContainerType : uint8_t {
    UNKN=0,
    AVI=1,
    MOV=2,
};

class BaseDemuxer{
    public:
        BaseDemuxer(Files *files);
        ~BaseDemuxer();

        // Starts initial demux parsing. Uses file contents
        // to guess the container to demux (.avi or .mov).
        // Returns true if successful
        virtual bool begin() = 0;   // `= 0`, pure, must be implemented by derived class

        // After calling `.begin()`, call this to get encoded
        // video data on a chunk by chunk basis into `output`
        // https://developer.apple.com/standards/qtff-2001.pdf?#page=68
        // https://developer.apple.com/standards/qtff-2001.pdf?#page=77
        virtual size_t get_next_video_data(uint8_t *data_output, size_t data_output_size_limit) = 0;   // `= 0`, pure, must be implemented by derived class

        // After calling `.begin()`, call this to get audio
        // data on a chunk by chunk basis into `output`
        // https://developer.apple.com/standards/qtff-2001.pdf?#page=68
        // https://developer.apple.com/standards/qtff-2001.pdf?#page=77
        virtual size_t get_next_audio_data(uint8_t *data_output, size_t data_output_size_limit) = 0;   // `= 0`, pure, must be implemented by derived class
    protected:
        // Pointer to instance of files object for reading and seeking videos
        Files *files;
};

#endif