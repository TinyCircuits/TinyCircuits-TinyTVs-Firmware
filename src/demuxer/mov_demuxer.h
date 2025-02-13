#ifndef MOV_DEMUXER_H
#define MOV_DEMUXER_H

#include "base_demuxer.h"
#include "../files/files.h"

class MOVDemuxer : public BaseDemuxer{
    public:
        MOVDemuxer(Files *files);
        ~MOVDemuxer();

        bool begin() override;
        size_t get_next_video_chunk(uint8_t *output, size_t output_size) override;
        size_t get_next_audio_chunk(uint8_t *output, size_t output_size) override;
    private:
};

#endif