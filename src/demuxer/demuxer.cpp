#include "demuxer.h"
#include "../debug/debug.h"
#include <string.h>


Demuxer::Demuxer(Files *files) : BaseDemuxer(files), avi_demuxer(files), mov_demuxer(files){
    debug_println("Demuxer");

    // Store this
    this->files = files;

    // Set default
    container_type = ContainerType::UNKN;
}


Demuxer::~Demuxer(){

}


bool Demuxer::begin(){
    debug_println("Demuxer Begin");

    if(check_for_avi()){
        avi_demuxer.begin();
        return true;
    }

    if(check_for_mov()){
        mov_demuxer.begin();
        return true;
    }

    debug_println("ERROR: Could not determine container type!");
    container_type = ContainerType::UNKN;
    return false;
}


size_t Demuxer::get_next_video_chunk(uint8_t *output, size_t output_size){
    if(container_type == ContainerType::AVI){
        return avi_demuxer.get_next_video_chunk(output, output_size);
    }else if(container_type == ContainerType::MOV){
        return mov_demuxer.get_next_video_chunk(output, output_size);
    }

    return 0;
}


size_t Demuxer::get_next_audio_chunk(uint8_t *output, size_t output_size){
    if(container_type == ContainerType::AVI){
        return avi_demuxer.get_next_audio_chunk(output, output_size);
    }else if(container_type == ContainerType::MOV){
        return mov_demuxer.get_next_audio_chunk(output, output_size);
    }

    return 0;
}


// https://www.loc.gov/preservation/digital/formats/fdd/fdd000059.shtml
// https://learn.microsoft.com/en-us/windows/win32/directshow/avi-riff-file-reference
bool Demuxer::check_for_avi(){
    uint8_t buffer[4];

    // First 4 bytes of file should be ASCII characters `RIFF`
    files->read_video(buffer, 4);

    if(strncmp("RIFF", (const char*)buffer, 4) == 0){
        debug_println("SUCCESS: File contents indicate this is a .avi file!");
        container_type = ContainerType::AVI;
        return true;
    }

    return false;
}


// https://www.loc.gov/preservation/digital/formats/fdd/fdd000052.shtml
// https://wiki.multimedia.cx/index.php?title=QuickTime_container
bool Demuxer::check_for_mov(){
    uint8_t buffer[4];

    // After 4 atom size bytes, next 4 bytes should be ASCII `ftyp`
    // https://wiki.multimedia.cx/index.php?title=QuickTime_container#:~:text=tbd-,ftyp,-ftyp
    files->seek_video(4, SEEK_SET);
    files->read_video(buffer, 4);

    if(strncmp("ftyp", (const char*)buffer, 4) == 0){
        debug_println("SUCCESS: File contents indicate this is a .mov file!");
        container_type = ContainerType::MOV;
        return true;
    }

    return false;
}