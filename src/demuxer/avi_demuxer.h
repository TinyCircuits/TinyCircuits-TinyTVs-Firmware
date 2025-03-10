#ifndef AVI_DEMUXER_H
#define AVI_DEMUXER_H

#include "base_demuxer.h"
#include "../files/files.h"

class AVIDemuxer : public BaseDemuxer{
    public:
        AVIDemuxer(Files *files);
        ~AVIDemuxer();

        bool begin() override;
        size_t get_next_video_data(uint8_t *output, size_t output_size) override;
        size_t get_next_audio_data(uint8_t *output, size_t output_size) override;
    private:
};

#endif