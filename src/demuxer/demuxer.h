#ifndef DEMUXER_H
#define DEMUXER_H

#include "../files/files.h"


enum ContainerType : uint8_t {
    UNKN=0,
    AVI=1,
    MOV=2,
};


class Demuxer{
    public:
        Demuxer(Files *files);
        ~Demuxer();

        // Starts initial demux parsing. Uses file contents
        // to guess the container to demux (.avi or .mov).
        // Returns true if successful
        bool begin();

        // After calling `.begin()`, call this to get encoded
        // video data on a chunk by chunk basis into `output`
        size_t get_next_video_chunk(uint8_t *output, size_t output_size);

        // After calling `.begin()`, call this to get audio
        // data on a chunk by chunk basis into `output`
        size_t get_next_audio_chunk(uint8_t *output, size_t output_size);
    private:
        // These functions are called from `.begin()` to check
        // the files contents for information indicating the
        // file is of a certain container type. Depending on
        // type, certain audio and video codecs are assumed
        // Returns try if figured out type, false otherwise
        bool check_for_avi();
        bool check_for_mov();

        Files *files;

        uint8_t container_type;
};

#endif