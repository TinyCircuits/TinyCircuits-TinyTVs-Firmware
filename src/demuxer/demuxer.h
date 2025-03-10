#ifndef DEMUXER_H
#define DEMUXER_H

#include "../files/files.h"
#include "base_demuxer.h"
#include "mov_demuxer.h"
#include "avi_demuxer.h"


class Demuxer : public BaseDemuxer{
    public:
        Demuxer(Files *files);
        ~Demuxer();

        bool begin() override;
        size_t get_next_video_data(uint8_t *output, size_t output_size) override;
        size_t get_next_audio_data(uint8_t *output, size_t output_size) override;
    private:
        // These functions are called from `.begin()` to check
        // the files contents for information indicating the
        // file is of a certain container type. Depending on
        // type, certain audio and video codecs are assumed
        // Returns try if figured out type, false otherwise
        bool check_for_avi();
        bool check_for_mov();

        // Container type discovered on `.begin()` call
        uint8_t container_type;

        // Specific container demuxers for holding unique
        // logic and state per container type
        MOVDemuxer mov_demuxer;
        AVIDemuxer avi_demuxer;
};

#endif