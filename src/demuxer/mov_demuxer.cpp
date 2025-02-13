#include "mov_demuxer.h"
#include "../debug/debug.h"
#include <stdint.h>
#include <stdio.h>

#define SWAP_INT_16(x) ((((x) & 0xff00) >> 8) | (((x) & 0x00ff) << 8))
#define SWAP_INT_32(x) ((SWAP_INT_16(x) << 16) | (SWAP_INT_16((x) >> 16)))


// https://developer.apple.com/standards/qtff-2001.pdf?#page=19
typedef struct atom_header_t{
    uint32_t size;
    char type[4];
}atom_header_t;


MOVDemuxer::MOVDemuxer(Files *files) : BaseDemuxer(files){
    debug_println("MOV Demuxer");
}


MOVDemuxer::~MOVDemuxer(){

}


bool MOVDemuxer::begin(){
    debug_println("MOV Demuxer Begin");

    off_t size   = files->video_size();
    off_t offset = files->video_seek(0, SEEK_SET); // Make sure we're at the start of the video
    
    atom_header_t header;

    while(offset < size){
        files->video_read((uint8_t *)(&header), sizeof(header));

        header.size = SWAP_INT_32(header.size);

        debug_println(header.size);
        debug_println(header.type);

        offset = files->video_seek(header.size - sizeof(header), SEEK_CUR);
    }

    return false;
}


size_t MOVDemuxer::get_next_video_chunk(uint8_t *output, size_t output_size){
    return 0;
}


size_t MOVDemuxer::get_next_audio_chunk(uint8_t *output, size_t output_size){
    return 0;
}