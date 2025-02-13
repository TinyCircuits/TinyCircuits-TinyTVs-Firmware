#include "avi_demuxer.h"
#include "../debug/debug.h"
#include <stdint.h>
#include <stdio.h>


AVIDemuxer::AVIDemuxer(Files *files) : BaseDemuxer(files){
    debug_println("AVI Demuxer");
}


AVIDemuxer::~AVIDemuxer(){
    
}


bool AVIDemuxer::begin(){
    debug_println("AVI Demuxer Begin");
    return false;
}


size_t AVIDemuxer::get_next_video_chunk(uint8_t *output, size_t output_size){
    return 0;
}


size_t AVIDemuxer::get_next_audio_chunk(uint8_t *output, size_t output_size){
    return 0;
}