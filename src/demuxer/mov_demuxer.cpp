#include "mov_demuxer.h"
#include "../debug/debug.h"
#include <stdio.h>
#include <string.h>

#define SWAP_INT_16(x) ((((x) & 0xff00) >> 8) | (((x) & 0x00ff) << 8))
#define SWAP_INT_32(x) ((SWAP_INT_16(x) << 16) | (SWAP_INT_16((x) >> 16)))


MOVDemuxer::MOVDemuxer(Files *files) : BaseDemuxer(files){
    debug_println("MOV Demuxer");
}


MOVDemuxer::~MOVDemuxer(){

}


// .mov files have blocks of data called "atoms" and each atom starts
// with 8 bytes of data: 4 bytes for size and 4 bytes for ASCII ID.
// Atoms can contain other atoms. A .mov file has a structure of atoms
// as follows (only showing atoms we care about):
//
//  Example:    https://github.com/epsi1on/QuicktimeAtomParserNet?tab=readme-ov-file#qicktime-atom-parser-in-net
//  Atom types: https://developer.apple.com/standards/qtff-2001.pdf?#page=267
//
//  * mdat                                                                                              https://developer.apple.com/standards/qtff-2001.pdf?#page=26
//      * audio and video chunks referenced in "trak" atoms
//  * moov                                                                                              https://developer.apple.com/standards/qtff-2001.pdf?#page=31
//      * mvhd  <- `duration`, `time scale`                                                             https://developer.apple.com/standards/qtff-2001.pdf?#page=33
//      * trak  <- audio or video                                                                       https://developer.apple.com/standards/qtff-2001.pdf?#page=39
//          * tkhd <- `track width`, `track height`, `track volume` (can be either video or sound)      https://developer.apple.com/standards/qtff-2001.pdf?#page=41
//          * mdia                                                                                      https://developer.apple.com/standards/qtff-2001.pdf?#page=55
//              * mdhd <- `duration`, `time scale`                                                      https://developer.apple.com/standards/qtff-2001.pdf?#page=55
//              * minf <- `smhd` or `vmhd` header for sound or video and contains `stbl`                https://developer.apple.com/standards/qtff-2001.pdf?#page=58
//                  * stbl <-                                                                           https://developer.apple.com/standards/qtff-2001.pdf?#page=68
//                      * stco                                                                          https://developer.apple.com/standards/qtff-2001.pdf?#page=78
//      * trak  <- audio or video                                                                       https://developer.apple.com/standards/qtff-2001.pdf?#page=39
//          * tkhd <- `track width`, `track height`, `track volume` (can be either video or sound)      https://developer.apple.com/standards/qtff-2001.pdf?#page=41
//          * mdia                                                                                      https://developer.apple.com/standards/qtff-2001.pdf?#page=55
//              * mdhd <- `duration`, `time scale`                                                      https://developer.apple.com/standards/qtff-2001.pdf?#page=55
//              * minf <- `smhd` or `vmhd` header for sound or video and contains `stbl`                https://developer.apple.com/standards/qtff-2001.pdf?#page=58
//                  * stbl <-                                                                           https://developer.apple.com/standards/qtff-2001.pdf?#page=68
//                      * stco                                                                          https://developer.apple.com/standards/qtff-2001.pdf?#page=78
bool MOVDemuxer::begin(){
    debug_println("MOV Demuxer Begin");

    // Get size of entire file
    off_t video_file_size = files->video_size();

    // Parse file for offsets and info needed for playing the video and audio
    find_mdat_moov_atoms(video_file_size);
    find_and_parse_trak_atoms();

    return false;
}


size_t MOVDemuxer::get_next_video_chunk(uint8_t *output, size_t output_size){
    return 0;
}


size_t MOVDemuxer::get_next_audio_chunk(uint8_t *output, size_t output_size){
    return 0;
}


void MOVDemuxer::read_atom_header(atom_header_t *header){
    // Read atom header and convert size from big to little endian
    files->video_read((uint8_t *)(header), ATOM_HEADER_SIZE);
    header->size = SWAP_INT_32(header->size);
}


off_t MOVDemuxer::find_atom(off_t start_offset, off_t end_offset, const char type[4], atom_header_t *output_header){
    debug_println("###");
    debug_println("Looking for atom start:");
    debug_println(type);

    // Set to location in files
    off_t offset = files->video_seek(start_offset, SEEK_SET);

    // Read until found or get to end
    while(offset < end_offset){
        // Read atom headers for size and type of high level atom
        read_atom_header(output_header);

        // https://developer.apple.com/standards/qtff-2001.pdf?#page=25
        if(strncmp(type, output_header->type, 4) == 0){
            debug_println("SUCCESS: Found atom!");
            debug_println(output_header->type);
            debug_println(output_header->size);

            debug_println("Looking for atom end, SUCCESS, FOUND IT:");
            debug_println("###");

            return offset + ATOM_HEADER_SIZE;
        }else{
            debug_println("Skipping atom:");
            debug_println(output_header->type);
            debug_println(output_header->size);
        }

        // Skip over all high level nested data/atoms
        offset = files->video_seek(output_header->size - ATOM_HEADER_SIZE, SEEK_CUR);
    }

    // Not found
    debug_println("Looking for atom end, ERROR, DID NOT FIND IT:");
    debug_println("###");

    return -1;
}


void MOVDemuxer::find_mdat_moov_atoms(off_t video_file_size){
    atom_header_t header;
    off_t after_header_offset = 0;

    // https://developer.apple.com/standards/qtff-2001.pdf?#page=25
    // Start at the beginning of the file
    after_header_offset = find_atom(0, video_file_size, "mdat", &header);                                                       //  https://developer.apple.com/standards/qtff-2001.pdf?#page=26
    if(after_header_offset != -1){
        mdat_start_offset = after_header_offset;                                                                                // Store offset to just after atom header
        mdat_end_offset   = after_header_offset + header.size - ATOM_HEADER_SIZE;
    }

    // Start at `mdat` offset but then skip it and start after
    // it using size of atom from header (assuming an atom order)
    after_header_offset = find_atom(after_header_offset + header.size - ATOM_HEADER_SIZE, video_file_size, "moov", &header);    // https://developer.apple.com/standards/qtff-2001.pdf?#page=31
    if(after_header_offset != -1){
        moov_start_offset = after_header_offset;                                                                                // Store offset to just after atom header
        moov_end_offset   = after_header_offset + header.size - ATOM_HEADER_SIZE;
    }
}


void MOVDemuxer::find_and_parse_trak_atoms(){
    atom_header_t header;
    off_t after_header_offset = 0;

    // Start at `moov` offset but then skip into it and start at
    // nested atom by skipping header into next header
    after_header_offset = find_atom(moov_start_offset, moov_end_offset, "mvhd", &header);   // https://developer.apple.com/standards/qtff-2001.pdf?#page=33
    if(after_header_offset != -1){
        // Seek past irelevent info
        files->video_seek(1 + 3 + 4 + 4, SEEK_CUR);
        files->video_read((uint8_t *)(&general_time_scale), 4);
        files->video_read((uint8_t *)(&general_duration), 4);
        
        general_time_scale = SWAP_INT_32(general_time_scale);
        general_duration   = SWAP_INT_32(general_duration);

        debug_println("Time scale and duration:");
        debug_println(general_time_scale);
        debug_println(general_duration);
    }

    // Start at `mvhd` offset but then skip over it and start at
    // next atom by skipping using size of atom
    after_header_offset = find_atom(after_header_offset + header.size - ATOM_HEADER_SIZE, moov_end_offset, "trak", &header);    // https://developer.apple.com/standards/qtff-2001.pdf?#page=39
    if(after_header_offset != -1){
        parse_tkhd_atom(after_header_offset);
        find_and_prase_mdia(after_header_offset);
    }
    
    // Start at `trak` offset but then skip over it and start at
    // next atom by skipping using size of atom (skipping last trak)
    after_header_offset = find_atom(after_header_offset + header.size - ATOM_HEADER_SIZE, moov_end_offset, "trak", &header);    // https://developer.apple.com/standards/qtff-2001.pdf?#page=39
    if(after_header_offset != -1){
        parse_tkhd_atom(after_header_offset);
        find_and_prase_mdia(after_header_offset);
    }
}


void MOVDemuxer::parse_tkhd_atom(off_t after_header_offset){
    atom_header_t header;

    // Make sure `tkhd` is found right away (after `trak` atom header)
    // https://developer.apple.com/standards/qtff-2001.pdf?#page=41
    after_header_offset = find_atom(after_header_offset, moov_end_offset, "tkhd", &header);
    if(after_header_offset != -1){
        // Seek past irelevent info to `Duration`
        uint32_t track_duration = 0;
        files->video_seek(1 + 3 + 4 + 4 + 4 + 4, SEEK_CUR);
        files->video_read((uint8_t *)(&track_duration), 4);

        // Seek past irelevent info to `Volume`
        uint16_t volume = 0;
        files->video_seek(8 + 2 + 2, SEEK_CUR);
        files->video_read((uint8_t *)(&volume), 2);

        // Seek past irelevent info to `Track width` and `Track height`
        uint32_t track_width = 0;
        uint32_t track_height = 0;
        files->video_seek(2 + 36, SEEK_CUR);
        files->video_read((uint8_t *)(&track_width), 4);
        files->video_read((uint8_t *)(&track_height), 4);

        track_duration = SWAP_INT_32(track_duration);
        volume         = SWAP_INT_32(volume);
        track_width    = SWAP_INT_16(track_width);  // Not sure why 16-bit swap needed instead of 32-bit, it is read as 4 bytes, so, what?
        track_height   = SWAP_INT_16(track_height);

        debug_println("Track duration, volume, width, and height:");
        debug_println(track_duration);
        debug_println(volume);
        debug_println(track_width);
        debug_println(track_height);

        if(track_width == 0 && track_height == 0){  // Must be a audio track!
            trak_audio_duration = track_duration;
            trak_audio_volume = volume;
        }else{                                      // Must be a video track!
            trak_video_duration = track_duration;
            trak_video_width_px = track_width;
            trak_video_height_px = track_height;
        }
    }
}


void MOVDemuxer::parse_stco_atom(off_t after_header_offset, uint32_t *entry_count, off_t *first_chunk_offset){

}


void MOVDemuxer::find_and_prase_mdia(off_t after_header_offset){
    atom_header_t header;

    // Starting on what is likely the `tkhd` atom header,
    // find and skip atoms until we find `mdia` atom
    after_header_offset = find_atom(after_header_offset, moov_end_offset, "mdia", &header);
    if(after_header_offset == -1) return;

    // Must have found the `mdia` atom, now find the child `minf` atom
    after_header_offset = find_atom(after_header_offset, moov_end_offset, "minf", &header);
    if(after_header_offset == -1) return;

    // Must have found the `minf` atom, now find the child `vmhd` or `smhd` atom
    off_t after_vmhd_atom_header_offset = find_atom(after_header_offset, moov_end_offset, "vmhd", &header);
    if(after_vmhd_atom_header_offset != -1){
        off_t after_stbl_atom_header_offset = find_atom(after_header_offset, moov_end_offset, "stbl", &header);
        off_t after_stco_atom_header_offset = find_atom(after_stbl_atom_header_offset, moov_end_offset, "stco", &header);
        parse_stco_atom(after_stco_atom_header_offset, &trak_video_chunk_count, &trak_video_chunk_offset);
        return;
    }

    off_t after_smhd_atom_header_offset = find_atom(after_header_offset, moov_end_offset, "smhd", &header);
    if(after_smhd_atom_header_offset != -1){
        off_t after_stbl_atom_header_offset = find_atom(after_header_offset, moov_end_offset, "stbl", &header);
        off_t after_stco_atom_header_offset = find_atom(after_stbl_atom_header_offset, moov_end_offset, "stco", &header);
        parse_stco_atom(after_stco_atom_header_offset, &trak_audio_chunk_count, &trak_audio_chunk_offset);
        return;
    }
}