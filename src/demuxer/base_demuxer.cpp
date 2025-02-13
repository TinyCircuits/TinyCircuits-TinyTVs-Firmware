#include "base_demuxer.h"
#include "../debug/debug.h"


BaseDemuxer::BaseDemuxer(Files *files){
    debug_println("Base Demuxer");

    // Store this
    this->files = files;
}


BaseDemuxer::~BaseDemuxer(){

}