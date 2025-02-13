#include "mov_demuxer.h"
#include "../debug/debug.h"
#include <stdint.h>
#include <stdio.h>


MOVDemuxer::MOVDemuxer(Files *files) : BaseDemuxer(files){
    debug_println("MOV Demuxer");
}


MOVDemuxer::~MOVDemuxer(){

}


bool MOVDemuxer::begin(){
    debug_println("MOV Demuxer Begin");
    return false;
}


size_t MOVDemuxer::get_next_video_chunk(uint8_t *output, size_t output_size){
    return 0;
}


size_t MOVDemuxer::get_next_audio_chunk(uint8_t *output, size_t output_size){
    return 0;
}