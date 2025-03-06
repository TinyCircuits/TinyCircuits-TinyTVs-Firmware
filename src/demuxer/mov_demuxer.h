#ifndef MOV_DEMUXER_H
#define MOV_DEMUXER_H

#include "base_demuxer.h"
#include "../files/files.h"
#include <stdint.h>


// https://developer.apple.com/standards/qtff-2001.pdf?#page=19
typedef struct atom_header_t{
    uint32_t size;
    char type[4];
}atom_header_t;

#define ATOM_HEADER_SIZE sizeof(atom_header_t)

class MOVDemuxer : public BaseDemuxer{
    public:
        MOVDemuxer(Files *files);
        ~MOVDemuxer();

        bool begin() override;

        size_t get_next_video_data(uint8_t *data_output, size_t data_output_size_limit) override;
        size_t get_next_audio_data(uint8_t *data_output, size_t data_output_size_limit) override;
    private:
        // Assuming the current video file is at an atom header start offset, read the header
        void read_atom_header(atom_header_t *header);

        // Starting at a specific offset in the file, find an
        // atom assuming we're starting at an atom beginning
        // Searches for atom with `type `until >= `end_offset`
        // and returns offset to after atom header of `type`
        // or returns `-1` if not found
        off_t find_atom(off_t start_offset, off_t end_offset, const char type[4], atom_header_t *output_header);

        // Finds and stores offsets to high level movie data and movie info atoms
        void find_mdat_moov_atoms(off_t video_file_size);

        // Finds and stores offsets to video and audio atoms inside `moov` atom
        void find_and_parse_trak_atoms();

        // Parses track header atoms assuming the file is at
        // the start of a `trak` atom header
        void parse_tkhd_atom(off_t after_header_offset);

        // https://developer.apple.com/standards/qtff-2001.pdf?#page=76
        void parse_stsz_atom(off_t after_header_offset, uint32_t *number_of_entries, off_t *first_sample_offset);

        // https://developer.apple.com/standards/qtff-2001.pdf?#page=78
        void parse_stco_atom(off_t after_header_offset, uint32_t *number_of_entries, off_t *first_chunk_offset);

        void find_and_prase_mdia(off_t after_header_offset);

        // Offsets to movie data and info .mov atoms: https://developer.apple.com/standards/qtff-2001.pdf?#page=25
        // Set in `find_mdat_moov_atoms(...)`
        uint32_t mdat_start_offset;   // Offset to just after atom header
        uint32_t mdat_end_offset;     // Offset to atom after `mdat`

        uint32_t moov_start_offset;   // Offset to just after atom header
        uint32_t moov_end_offset;     // Offset to atom after `moov`

        // `mvhd` movie time scale and duration: https://developer.apple.com/standards/qtff-2001.pdf?#page=34
        // Set in `find_and_parse_trak_atoms(...)`
        uint32_t general_time_scale;
        uint32_t general_duration;

        // Width and height of the video referenced in `trak_video_offset`: https://developer.apple.com/standards/qtff-2001.pdf?#page=41
        // Set in `find_and_parse_tkhd_atoms(...)`
        uint32_t trak_video_width_px;
        uint32_t trak_video_height_px;

        // Volume of the audio referenced in `trak_audio_offset`: https://developer.apple.com/standards/qtff-2001.pdf?#page=41
        // Set in `find_and_parse_tkhd_atoms(...)`
        uint16_t trak_audio_volume;

        // Offsets to movie tracks that indicate locations of video and audio chunks in `mdat`: https://developer.apple.com/standards/qtff-2001.pdf?#page=39
        // Set in `find_and_parse_tkhd_atom(...)`
        uint32_t trak_video_duration;           // Duration of video track (can be different from audio)
        uint32_t trak_video_stbl_offset;        // Offset is to after `stbl` atom header

        uint32_t trak_video_chunk_count;
        off_t trak_video_chunk_offset;          // Offset to start of first video media chunk and sample

        uint32_t trak_video_sample_count;
        off_t trak_video_sample_size_offset;    // Offset to start of video sample size table


        uint32_t trak_audio_duration;           // Duration of audio track (can be different from video)
        uint32_t trak_audio_stbl_offset;        // Offset is to after `stbl` atom header

        uint32_t trak_audio_chunk_count;
        off_t trak_audio_chunk_offset;          // Offset to start of first audio media chunk and sample

        uint32_t trak_audio_sample_count;
        off_t trak_audio_sample_size_offset;    // Offset to start of audio sample size table
};

#endif