//-------------------------------------------------------------------------------
//  TinyCircuits TinyTV Firmware
//
//  Changelog:
//  03/25/2026 MP4 playback update
//  05/26/2023 Initial Release for TinyTV 2/Mini
//  02/08/2023 Cross-platform base committed
//
//  Written by Mason Watmough, Ben Rose, and Jason Marcum for TinyCircuits, http://TinyCircuits.com
//
//-------------------------------------------------------------------------------

const int maxVideos = MAX_VIDEO_FILES;
char aviList[maxVideos][13] = {"\0"};
int aviCount = 0;
uint32_t livePos;
uint8_t nextChunkTag[8];
bool videoStreamReadyAVI = false;
bool videoStreamReadyTSV = false;
bool videoStreamReadyMP4 = false;
uint64_t tsMillisInitial = 0;
uint32_t currentAudioRate = 0;
int videoFormat = 0;
uint64_t millisOffset = 0;
uint32_t aviRIFFSize = 0;
int aviMoviListCount = 0;
int aviMoviListPos = 0;
uint32_t aviMoviListOffsets[5];

uint32_t getIntBE(uint8_t * intOffset) {
  return (uint32_t(intOffset[3]) << 24) | (uint32_t(intOffset[2]) << 16) | (uint32_t(intOffset[1]) << 8) | (uint32_t(intOffset[0]));
}

uint64_t getMillisOffset() { return millisOffset; }
void setMillisOffset(uint64_t val) { millisOffset = val; }

#ifndef TinyTVKit

#include <pico/mutex.h>

mutex_t flacLoadMtx;

uint32_t mp4moovSize = 0;

uint8_t trafBuf1[1024];
uint8_t trafBuf0[1024];
uint8_t h264Buf[12288];
uint8_t flacBuf[8192];
uint16_t h264SampleOffsets[256];
uint16_t flacSampleOffsets[256];
int h264DefaultSampleSize = 0;
int flacDefaultSampleSize = 0;

int trafOffset;
int trafBufStart;
int H264trafBufStart;
int trafSize;
int tfhdBase;
int H264tfhdBase;
int trunFlags = 0;
int trunSamplesStart;
int H264SamplesStart;
int trunNSamples;
int H264NSamples;
int H264TrafCurrentSample = 0;
int sampleSizeOffset;
int H264SampleSizeOffset;
int H264trafBytesPerSample;
int trafBytesPerSample;
int H264trafDefaultSampleSize;
int trafBOffset = 0;
int trafSamplesReady = 0;
int trafDefaultSampleSize = 0;
int sampleDefaultDuration = 512;
int trafTrack = 0;
int H264Ready = 0;
int FLACReady = 0;

int moofOffset = 0;
int moofSize = 0;
int mdatOffset = 0;
int mdatSize = 0;
int haveMoof = 0;
int haveMdat = 0;
int timescaleDuration = 0;
int MP4Timescale = 0;
int MP4ToplevelError = 0;
int loadFLACTraf = 0;
int loadH264Traf = 1;
int mp4SeekMS = 0;
int currentFLACDataChunk = 0;
int numFLACDataChunks = 0;

bool isMdat() { return haveMdat; }
bool isMoof() { return haveMoof; }
bool getH264Ready() { return H264Ready; }
bool getFLACReady() { return FLACReady; }
int getTrafOffset() { return trafOffset; }
int getTrafSize() { return trafSize; }
int getMoofOffset() { return moofOffset; }
int getMoofSize() { return moofSize; }
int getMdatOffset() { return mdatOffset; }
int getMdatSize() { return mdatSize; }
int getReadOffset() { return trafBOffset; }
int getMP4ToplevelError() { return MP4ToplevelError; }
int getTrafSamplesReady() { return trafSamplesReady; }
int getTrafTrack() { return trafTrack; }
int getTrunSampleCount() { return trunNSamples; }
int getH264SampleCount() { return H264NSamples; }
int getH264TrafCurrentSample() { return H264TrafCurrentSample; }
int getTimescaleDuration() { return timescaleDuration; }
uint8_t* getH264TrafBuf() { return trafBuf0; }
uint8_t* getFLACTrafBuf() { return trafBuf1; }
uint8_t* getFLACBuf() { return flacBuf; }

int H264Decoded = 1;

int getH264DecodeReady() { return !H264Decoded; }
void setH264DecodeDone() { H264Decoded = 1; }
void setH264DecodeReady() { H264Decoded = 0; }

void FLACUsed() { FLACReady = 0; }
void H264Used() { H264Ready = 0; }
void trafSamplesUsed() { trafSamplesReady = 0; }
void resetMP4ToplevelError() { MP4ToplevelError = 0; }

void initFlacMtx() { mutex_init(&flacLoadMtx); }

void resetH264Traf() {
  dbgPrint("h264 reset!");
  H264TrafCurrentSample = 0;
  loadH264Traf = 1;
}

void advanceH264TrafCurrentSample() { H264TrafCurrentSample++; }

uint32_t getIntLE(uint8_t* p) {
  uint32_t ret = ((uint32_t)(*p) & 0xff) << 24;
  ret |= (uint32_t)(*(p+1) & 0xff) << 16;
  ret |= (uint32_t)(*(p+2) & 0xff) << 8;
  return ret | (uint32_t)(*(p+3) & 0xff);
}

int compareFourCC(uint8_t* p, const char* fourcc) {
  for(int i = 0; i < 4; i++) {
    if(p[i] != fourcc[i]) return 0;
  }
  return 1;
}

uint64_t getLongLE(uint8_t* p) {
  uint64_t ret = ((uint64_t)(*p) & 0xff) << 56;
  ret |= (uint64_t)(*(p+1) & 0xff) << 48;
  ret |= (uint64_t)(*(p+2) & 0xff) << 40;
  ret |= (uint64_t)(*(p+3) & 0xff) << 32;
  ret |= (uint64_t)(*(p+4) & 0xff) << 24;
  ret |= (uint64_t)(*(p+5) & 0xff) << 16;
  ret |= (uint64_t)(*(p+6) & 0xff) << 8;
  return ret | (uint64_t)(*(p+7) & 0xff);
}

int getMP4VideoInfo(int startTimeOffsetS, storage_t* h264Dec, fx_flac_t* flacDec) {

  moofOffset = 0;
  moofSize = 0;
  mdatOffset = 0;
  mdatSize = 0;
  haveMoof = 0;
  haveMdat = 0;
  mp4moovSize = 0;

  if(videoBuf[1] != NULL && DOUBLE_BUFFER) {
    free(videoBuf[1]);
    videoBuf[1] = NULL;
  }

  uint8_t readBuf[2048];
  uint8_t hdrBuf[256];
  dbgPrint("Reading from offset "+String(infile.position()));
  int read = infile.read(readBuf, 256);
  int moovOffset = 0;
  int offset = 0;
  int foundMoov = 0;
  int foundavcC = 0;
  int foundFLAC = 0;
  while(read > 0) {
    for(int i = 0; i < 256-8; i++) {
      if(compareFourCC(readBuf+i, "moov")) {
        moovOffset = offset+i;
        mp4moovSize = getIntLE(readBuf+i-4);
        foundMoov = 1;
        break;
      }
    }
    if(foundMoov) break;
    for(int i = 0; i < 8; i++) {
      readBuf[i] = readBuf[256-8+i];
    }
    offset += read;
    read = infile.read(readBuf+8, 256-8);
  }
  
  infile.seekSet(moovOffset);

  offset = infile.position();

  infile.read(readBuf, 2048);

  for(int i = 0; i < 2048-8; i++) {
    if(compareFourCC(readBuf+i, "mdhd")) {
      int mdhd_offset = i;
      dbgPrint("Found mdhd box at "+String(mdhd_offset));
      MP4Timescale = getIntLE(readBuf+i+16);
      break;
    }
  }

  int avcC_offset = 0;
  int avcC_size = 0;

  uint8_t* hdrBufWrite = hdrBuf;

  int sps_size = 0;
  int pps_size = 0;

  while(read > 0) {
    for(int i = 0; i < 2048-8; i++) {
      if(compareFourCC(readBuf+i, "avcC")) {
        int avcC_offset = offset+i;
        dbgPrint("Found avcC box at "+String(avcC_offset));
        int avcC_size = getIntLE(readBuf+i-4);

        if(avcC_size <= 2048-i) {
          dbgPrint("Got full avcC box!\n\r");
          for(int j = i; j < i + avcC_size+4; j++) {
            if(readBuf[j] == 0x67) {
              sps_size = ((uint32_t)readBuf[j-2] << 8) | ((uint32_t)readBuf[j-1]);
              dbgPrint("sps_size is "+String(sps_size));
              /* Fix AVCC NAL unit */
              hdrBufWrite[0] = 0;
              hdrBufWrite[1] = 0;
              hdrBufWrite[2] = 0;
              hdrBufWrite[3] = 1;
              for(int k = 0; k < sps_size; k++) {
                hdrBufWrite[k+4] = readBuf[j+k];
              }
              hdrBufWrite += 4+sps_size;
            } else if(readBuf[j] == 0x68) {
              pps_size = ((uint32_t)readBuf[j-2] << 8) | ((uint32_t)readBuf[j-1]);
              dbgPrint("pps_size is "+String(pps_size));
              /* Fix AVCC NAL unit */
              hdrBufWrite[0] = 0;
              hdrBufWrite[1] = 0;
              hdrBufWrite[2] = 0;
              hdrBufWrite[3] = 1;
              for(int k = 0; k < pps_size; k++) {
                hdrBufWrite[k+4] = readBuf[j+k];
              }
              hdrBufWrite += 4+pps_size;
            }
          }
        }
        foundavcC = 1;
        break;
      }
    }
    if(foundavcC) break;
    for(int i = 0; i < 8; i++) {
      readBuf[i] = readBuf[2048-8+i];
    }
    offset += read;
    read = infile.read(readBuf+8, 2048-8);
  }

  uint32_t w = 216;
  uint32_t h = 135;
  uint8_t* outbuf_p = NULL;
  resetH264Traf();
  uint32_t ret_code = h264bsdDecode(h264Dec, hdrBuf, sps_size+pps_size+8, &outbuf_p, (u32*)(&w), (u32*)(&h));
  dbgPrint("H264 decoder init returned "+String(ret_code));

  infile.seekSet(moovOffset);

  offset = infile.position();

  dbgPrint("Reading from offset "+String(infile.position()));
  read = infile.read(readBuf, 2048);

  uint32_t flac_offset = 0;
  uint32_t flacSize = 0;

  while(read > 0) {
    for(int i = 0; i < 2048-8; i++) {
      if(compareFourCC(readBuf+i, "dfLa")) {
        flac_offset = offset+i;
        dbgPrint("Found dfLa box at "+String(flac_offset));
        flacSize = getIntLE(readBuf+i-4);

        if(flacSize <= 2048-i) {
          printf("Got full fLaC box!\n\r");

          flacSize = flacSize+4;
          for(int j = i; j < i + flacSize+4; j++) {
            hdrBuf[j-i] = readBuf[j];
          }
        }
        foundFLAC = 1;
        break;
      }
    }
    if(foundFLAC) break;
    for(int i = 0; i < 8; i++) {
      readBuf[i] = readBuf[2048-8+i];
    }
    offset += read;
    read = infile.read(readBuf+8, 2048-8);
  }

  hdrBuf[4] = 'f';
  hdrBuf[5] = 'L';
  hdrBuf[6] = 'a';
  hdrBuf[7] = 'C';
  int retv;
  int32_t tempBuf[4];
  uint32_t tempBufSize = sizeof(tempBuf)/sizeof(int32_t);
  
  if((retv = fx_flac_process(flacDec, hdrBuf+4, &flacSize, tempBuf, &tempBufSize)) == FLAC_ERR) {
    // Failed to parse FLAC header
  } else if(tempBufSize > 0) {
    // FLAC samples... from the header??
  } else {
    // No FLAC data, but normal decoder state
  }
  dbgPrint("FLAC decoder init returned "+String(retv));

  currentAudioRate = fx_flac_get_streaminfo(flacDec, FLAC_KEY_SAMPLE_RATE);
  targetFrameTime = 1000000 / 24; // Just as a default, corrected when first track fragments encountered
    
  videoStreamReadyMP4 = true;
  
  resetMP4SeekTime();
  //seekTimeMP4(0);
  seekTimeMP4(1000*startTimeOffsetS);
  
  return 1;
}

int totalAudioBufferDrops = 0;

int loadFLACDataChunk() {

  uint32_t s_size;

  int32_t sampleBuf[512];
  uint32_t sampleBufSize = 512;

  if(currentFLACDataChunk >= numFLACDataChunks) return 1;

  if(!mutex_try_enter(&flacLoadMtx, NULL)) return 0;

  dbgPrint("Getting FLAC sample (core1) "+String(currentFLACDataChunk));
  
  getFLACSample(getFLACBuf(), currentFLACDataChunk, &s_size);

  int retv;
  uint64_t t0 = micros();
  if((retv = fx_flac_process(decFlac, getFLACBuf(), &s_size, sampleBuf, &sampleBufSize)) == FLAC_ERR) {
    // Bad FLAC decoder state
  } else {
    // Normal FLAC decoder state
  }

  uint64_t t1 = micros();

  dbgPrint("(core1) flac chunk decode in "+String((int)(t1-t0)));

 if(getAudioSampleCount() < 16) {
   totalAudioBufferDrops++;
 }


  for(int i = 0; i < sampleBufSize; i++) {
    while(!pushAudioSample((sampleBuf[i] >> 24) + 128)) {yield();}
  }

  currentFLACDataChunk++;

  mutex_exit(&flacLoadMtx);
  
  return 0;
}

int loadMP4FLACData() {
  dbgPrint(String(getTrunSampleCount())+" flac chunks");
  numFLACDataChunks = getTrunSampleCount();
  currentFLACDataChunk = 0;
  return 1;
}

int getMP4NextTopLevel() {
  int offset = infile.position();
  uint8_t toplevelBuf[256];
  dbgPrint("(moofseek) Reading from offset "+String(offset));
  int read = infile.read(toplevelBuf, 256);
  haveMoof = haveMdat = 0;
  for(int i = 0; i < 256-8; i++) {
    if(compareFourCC(toplevelBuf+i, "moof")) {
      moofOffset = offset+i;
      moofSize = getIntLE(toplevelBuf+i-4);
      dbgPrint("Found moof at "+String(offset+i)+" with size "+String(moofSize));
      haveMoof = 1;
      return 0;
    } else if(compareFourCC(toplevelBuf+i, "mdat")) {
      mdatOffset = offset+i;
      mdatSize = getIntLE(toplevelBuf+i-4);
      dbgPrint("Found mdat at "+String(offset+i)+" with size "+String(mdatSize));
      haveMdat = 1;
      return 0;
    }
  }
  dbgPrint("Failed to get toplevel box!!");
  MP4ToplevelError = 1;
  return 1;
}

void seekTimeMP4(int seekMs) {
  int p0 = infile.position();
  infile.seekSet(infile.size()-4);
  uint8_t mfraOffBuf[4];
  infile.read(mfraOffBuf, 4);
  int mfraPos = getIntLE(mfraOffBuf);
  infile.seekCur(-mfraPos+20);
  uint8_t tfraBuf[12];
  
  infile.read(tfraBuf, 12);
  int tid = getIntLE(tfraBuf);
  int sizes = getIntLE(tfraBuf+4);
  int n = getIntLE(tfraBuf+8);

  int tfraElemSize = (
    16 
    + ((sizes & 0b11) + 1) 
    + (((sizes >> 2) & 0b11) + 1) 
    + (((sizes >> 4) & 0b11) + 1)
  );

  int r0 = infile.position();

  // Binary search to seek time, assuming offsets are sorted
  int l = 0;
  int r = n;
  uint8_t tfraElemBuf[tfraElemSize];
  infile.seekSet(r0 + tfraElemSize*(n-1));
  infile.read(tfraElemBuf, tfraElemSize);
  int tfraTime = timescaleDuration = getLongLE(tfraElemBuf);
  int moofOffset = getLongLE(tfraElemBuf+8);
  seekMs %= (tfraTime * sampleDefaultDuration / MP4Timescale);
  while(l < r) {
    int m = l + (r-l)/2;
    infile.seekSet(r0 + tfraElemSize*m);
    infile.read(tfraElemBuf, tfraElemSize);
    tfraTime = getLongLE(tfraElemBuf);
    moofOffset = getLongLE(tfraElemBuf+8);
    
    if(tfraTime < (seekMs * MP4Timescale / sampleDefaultDuration)) {
      l = m + 1;
    } else {
      r = m;
    }
    
    if(l >= r) {
      infile.seekSet(moofOffset);
      return;
    }
  }
  
  infile.seekSet(p0);
}

int loadMP4TrackFragment(int writeH264TrafInfo = 1) {
  int offset = infile.position();
  uint8_t trafBuf[512];
  dbgPrint("(trafseek) Reading from offset "+String(offset));
  int read = infile.read(trafBuf, 512);
  int tfhdFlags = 0;
  for(int i = 0; i < 512-8; i++) {
    
    if(compareFourCC(trafBuf+i, "moof")) {
      // Got next moof box earlier than expected
      dbgPrint("Found moof looking for track fragments!!");
      infile.seekSet(offset+i-4);
      return 0;
    }
    
    if(compareFourCC(trafBuf+i, "traf")) {
      trafOffset = offset+i;
      trafBufStart = i;
      trafSize = getIntLE(trafBuf+i-4);
      trafSamplesReady = 0;
      if(trafSize <= 512-trafBufStart) {
        trafSamplesReady = 1;
        trafTrack = getIntLE(trafBuf+trafBufStart+16);
        tfhdFlags = getIntLE(trafBuf+trafBufStart+12);
        trafBOffset = getLongLE(trafBuf+trafBufStart+20);
        trafDefaultSampleSize = getLongLE(trafBuf+i+28);

        tfhdBase = 68;

        trunFlags = getIntLE(trafBuf+trafBufStart+tfhdBase);

        trafBytesPerSample = 0;
        trunSamplesStart = 12;
        sampleSizeOffset = 0;

        trunNSamples = getIntLE(trafBuf+trafBufStart+tfhdBase+4);
        if(tfhdFlags & 1) { /* base-data-offset */ }
        if(tfhdFlags & 2) { /* SDI */ }

        if(tfhdFlags & 8) {
          /* default sample duration */
          sampleDefaultDuration = getIntLE(trafBuf+trafBufStart+28);
          targetFrameTime = 1000000 / (MP4Timescale / sampleDefaultDuration);
        }
        
        if(tfhdFlags & 16) { /* default sample size */ }
        if(tfhdFlags & 32) { /* default sample flags */ }

        if(trunFlags & 0x1) { /* trun data offset */
          trafBOffset += getIntLE(trafBuf+trafBufStart+tfhdBase+8);
        }

        if(trunFlags & 0x4) { /* trun first sample flags */
          trunSamplesStart += 4;
        }
        
        if(trunFlags & 0x100) { /* trun sample durations */
          sampleSizeOffset += 4;
          trafBytesPerSample += 4;
        }
        
        if(trunFlags & 0x200) { /* trun sample sizes */
          trafBytesPerSample += 4;
        }

        if(trunFlags & 0x400) { /* trun sample flags */ }
        if(trunFlags & 0x800) { /* sample composition time offsets */ }

        infile.seekSet(trafBOffset);

        if(trafTrack == 1) { // Video track
          if(writeH264TrafInfo) {
            memcpy(getH264TrafBuf(), trafBuf, 512);
            h264DefaultSampleSize = trunFlags & 0x200;
            H264NSamples = trunNSamples;
            H264SamplesStart = trunSamplesStart;
            H264tfhdBase = tfhdBase;
            H264trafBufStart = trafBufStart;
            H264trafBytesPerSample = trafBytesPerSample;
            H264SampleSizeOffset = sampleSizeOffset;
            H264trafDefaultSampleSize = trafDefaultSampleSize;
          }
          H264Ready = 1;
          dbgPrint("Got h264 traf!!");
          dbgPrint(String(H264NSamples) + " h264 samples");
        } else { // Audio track
          memcpy(getFLACTrafBuf(), trafBuf, 512);
          flacDefaultSampleSize = trunFlags & 0x200;
          FLACReady = 1;
        }

      }
      return 1;
    }
  }
  if(!trafSamplesReady) {
    dbgPrint("Did not get all track fragments!!");
  }
  return 1;
}

void resetMP4SeekTime() {
  mp4SeekMS = 0;
  loadH264Traf = 1;
  loadFLACTraf = 0;
}
 
void loadNextTraf() {
    if(mp4SeekMS >= 0) {
      dbgPrint("Loading initial fragments...");
      getMP4NextTopLevel();
      resetH264Traf();
      loadH264Traf = 1;
      mp4SeekMS = -1;
      
      skipToMoof();

      loadH264Traf = 1;
    }

    loadFLACDataChunk();

    if(loadH264Traf) {
      dbgPrint("Loading tracks!");
      loadMP4TrackFragment(1);
      uint64_t t0 = micros();
      skipToAndLoadH264Traf();
      uint64_t t1 = micros();
      dbgPrint("H264 skip+load in "+String((int)(t1-t0))+" us");
      
      skipToMoof();
      getMP4NextTopLevel();

      
      skipIfMdat();

      // Load FLAC data
      int ft0 = micros();

      int p0 = infile.position();

      loadMP4TrackFragment(0); // Don't overwrite h264 data
      
      if(getFLACReady()) {
        dbgPrint("Flushing FLAC samples...");
        while(!loadFLACDataChunk()) {}
        uint64_t t0 = micros();
        mutex_enter_blocking(&flacLoadMtx);
        currentFLACDataChunk = numFLACDataChunks = 0;
        skipToAndLoadFLACTraf();
        mutex_exit(&flacLoadMtx);
        uint64_t t1 = micros();
        dbgPrint("FLAC skip+load in "+String((int)(t1-t0))+" us");
        FLACUsed();
        int ft1 = micros(); 
      } else {
        // Got H264 data expecting FLAC data
        dbgPrint("Got h264 expecting FLAC...");

        infile.seekSet(p0); // Give up
        return;
      }

      getMP4NextTopLevel();
      
      skipIfMdat();
    }
}

void loadH264Samples() {
  int writeOffset = 0;
  int size = H264trafDefaultSampleSize;
  for(int i = 0; i < H264NSamples; i++) {
    if(h264DefaultSampleSize) {
      size = getIntLE(
        getH264TrafBuf()
        + H264trafBufStart
        + H264tfhdBase
        + H264SamplesStart
        + H264SampleSizeOffset
        + i * H264trafBytesPerSample
      );
    }
    h264SampleOffsets[i] = writeOffset;
    writeOffset += size;
  }
  dbgPrint("Reading "+String(writeOffset)+" h264 bytes");
  infile.read(h264Buf, writeOffset);
}

void getH264Sample(uint8_t* sampleBuf, int s, int* size) {
  if(h264DefaultSampleSize) {
    *size = getIntLE(
      getH264TrafBuf()
      + H264trafBufStart
      + H264tfhdBase
      + H264SamplesStart
      + H264SampleSizeOffset
      + s * H264trafBytesPerSample
    );
  } else {
    *size = H264trafDefaultSampleSize;
  }
  dbgPrint("Copying h264 at sample offset "+String(h264SampleOffsets[s])+" and size "+String(*size));
  memcpy(sampleBuf, h264Buf+h264SampleOffsets[s], *size);
}

uint8_t* getH264SamplePtr(int s, int* size) {
  if(s < 0) return NULL;
    if(h264DefaultSampleSize) {
    *size = getIntLE(
      getH264TrafBuf()
      + H264trafBufStart
      + H264tfhdBase
      + H264SamplesStart
      + H264SampleSizeOffset
      + s * H264trafBytesPerSample
    );
  } else {
    *size = H264trafDefaultSampleSize;
  }
  return h264Buf+h264SampleOffsets[s];
}

void loadFLACSamples() {
  uint64_t t0 = micros();
  int writeOffset = 0;
  int size = trafDefaultSampleSize;
  for(int i = 0; i < getTrunSampleCount(); i++) {
    if(flacDefaultSampleSize) {
      size = getIntLE(
        getFLACTrafBuf()
        + trafBufStart
        + tfhdBase
        + trunSamplesStart
        + sampleSizeOffset
        + i * trafBytesPerSample
      );
    }
    flacSampleOffsets[i] = writeOffset;
    writeOffset += size;
  }
  infile.read(flacBuf, writeOffset);
  uint64_t t1 = micros();
}

void getFLACSample(uint8_t* sampleBuf, int s, uint32_t* size) {
  if(flacDefaultSampleSize) {
    *size = getIntLE(
      getFLACTrafBuf()
      + trafBufStart
      + tfhdBase
      + trunSamplesStart
      + sampleSizeOffset
      + s * trafBytesPerSample
    );
  } else {
    *size = trafDefaultSampleSize;
  }
  memcpy(sampleBuf, flacBuf+flacSampleOffsets[s], *size);
}

void skipToMoof() {
  if(isMoof()) {
    infile.seekSet(getMoofOffset()+4);
  } else if (isMdat()) {

  }
}

void skipToAndLoadH264Traf() {
  if(getH264Ready()) {
    
    infile.seekSet(getReadOffset());
    loadH264Samples();
    infile.seekSet(getTrafOffset()+getTrafSize()-8);
    loadH264Traf = 0;
    
  }
}

void skipToAndLoadFLACTraf() {
  infile.seekSet(getReadOffset());
  loadFLACSamples();
  
  loadMP4FLACData();
}

void skipIfMdat() {
  if(isMdat()) {
    infile.seekSet(getMdatOffset()+getMdatSize()-4); 
    getMP4NextTopLevel();
    infile.seekSet(getMoofOffset()+4); 
  } else {
    infile.seekSet(getMoofOffset()+4); 
  }
}

void fixAVCCStream(uint8_t* buf, int bufsize) {
  uint8_t* sample_pos = buf;
        
  // Fix AVCC stream to annex-B format by swapping lengths for block delimiters
  while(sample_pos < buf + bufsize - 4) {
    
    int nal_size = getIntLE(sample_pos);
    if(nal_size == 0) break;
    sample_pos[0] = 0;
    sample_pos[1] = 0;
    sample_pos[2] = 0;
    sample_pos[3] = 1;
    sample_pos += nal_size+4;
  }
}

#endif

// AVI code start

int readNextChunk(uint8_t * dest, int maxLen) {
  int chunkLen = nextChunkLength();
  int chunkRead = 0;
  chunkRead = infile.read(dest, min(chunkLen, maxLen));

  if (chunkLen != chunkRead) {
    dbgPrint("chunkskip ");
    dbgPrint(String(chunkLen));
    dbgPrint(" ");
    dbgPrint(String(chunkRead));
    infile.seekCur(chunkLen - chunkRead);
  }

  if ((chunkLen & 1) != 0) { //padding
    infile.seekCur(1);
  }

  infile.read(nextChunkTag, 8);
  return chunkRead;
}

int skipChunk() {
  int chunkLen = nextChunkLength();
  infile.seekCur(chunkLen);

  if ((chunkLen & 1) != 0) { //padding
    infile.seekCur(1);
  }

  infile.read(nextChunkTag, 8);
  return 0;
}

bool printNextChunk() {
  //nextChunkTag[4] = 0;
  //dbgPrint((char *)nextChunkTag);
  return 0;
}
bool isNextChunkVideo() {
  return strncmp((char *)nextChunkTag + 0, "00dc", 4) == 0;
}

bool isNextChunkAudio() {
  return strncmp((char *)nextChunkTag + 0, "01wb", 4) == 0;
}

bool isNextChunkIndex() {
  return strncmp((char *)nextChunkTag + 0, "ix0", 3) == 0;//ix00,ix01
}

int nextChunkLength() {
  return getIntBE(nextChunkTag + 4);
}

void printHex(uint8_t * toPrint, int count) {
  String hex = "";
  for (int i = 0; i < count; i++) {
    hex += String(toPrint[i], HEX) + " ";
  }
  dbgPrint(hex);
  String ascii = "";
  for (int i = 0; i < count; i++) {
    ascii += String((char)toPrint[i]) + " ";
  }
  dbgPrint(ascii);
}

int jumpToNextMoviList() {
  dbgPrint("indexChunk");
  printHex(nextChunkTag, 8);
  skipChunk();//video index
  
  if (isNextChunkIndex()) {
    printHex(nextChunkTag, 8);
    skipChunk();//audio index
  }
  
  printHex(nextChunkTag, 8);
  if (strncmp((char *)nextChunkTag + 0, "idx1", 4) == 0) { // followed by AVIX size
    skipChunk();//audio index
    printHex(nextChunkTag, 8);
  }
  if (strncmp((char *)nextChunkTag + 0, "RIFF", 4) == 0) { // followed by AVIX size
    uint8_t chunkData[16] = {0};
    if (infile.read(chunkData, 16) != 16) {
      return 1;
    }
    printHex(chunkData, 16);
    if (strncmp((char *)chunkData + 0, "AVIXLIST", 8) == 0) { //followed by movi list size
      if (strncmp((char *)chunkData + 12, "movi", 4) == 0) { //followed by video data chunks
        infile.read(nextChunkTag, 8);
        printHex(nextChunkTag, 8);
        return 0;
      }
    }
  }
  return 1;
}

int skipToFrameAVI1_0(uint32_t MOVIsize, uint32_t framesToSkip) {
  uint32_t moviListStart = infile.curPosition();// + 8;
  uint32_t moviListOffset = 0;
  uint8_t chunkHeader[12];
  infile.seekCur(MOVIsize - 4);
  if (infile.read(chunkHeader, 8) != 8) {
    return 1;
  }
  if (strncmp((char *)chunkHeader, "idx1", 4) == 0) {

    uint8_t *tempBuf = sharedBuffer;
    int bufPos = 0;
    //Takes too long to actually read the whole index table, see how many KB can be skipped
    int framesPerKB = 0;
    if (infile.read(tempBuf, 1024) != 1024) {
      return 1;
    }
    while (bufPos < 1024) {
      if (strncmp((char *)tempBuf + bufPos, "00dc", 4) == 0) {
        framesPerKB++;
      }
      bufPos += 16;
    }
    framesToSkip -= framesPerKB;
    int KBtoSkip = framesToSkip / framesPerKB;
    infile.seekCur(KBtoSkip * 1024);
    framesToSkip -= (KBtoSkip * framesPerKB);
    if (framesToSkip < 1)framesToSkip = 1;

    //continue with 'exact' frame counting
    int unrecognizedChunk = 0;
    while (framesToSkip && unrecognizedChunk < 5) {
      bufPos = 0;
      int bytesRead = infile.read(tempBuf, 512);
      if (bytesRead != 512) {
        dbgPrint("error reading idx1");
        //don't error out, just use last found moviListOffset?
        //return 1;
        framesToSkip = 0;
      }
      while (framesToSkip && bufPos < bytesRead && unrecognizedChunk < 5) {
        if (strncmp((char *)tempBuf + bufPos, "00dc", 4) == 0) {
          unrecognizedChunk = 0;
          framesToSkip--;
          moviListOffset = getIntBE(tempBuf + bufPos + 8);
        } else {
          if (strncmp((char *)tempBuf + bufPos, "01wb", 4) == 0) {
            unrecognizedChunk = 0;
          } else {
            unrecognizedChunk++;
          }
        }
        bufPos += 16;
      }
      dbgPrint(String(framesToSkip));
    }
    if (unrecognizedChunk >= 5) {
      dbgPrint("Error reading idx1 chunk");
      moviListOffset = 0;
    }
    moviListOffset -= 4; //moviListStart already has first tag
  } else {
    dbgPrint("idx1 not at expected position");
  }
  infile.seekSet(moviListStart + moviListOffset);
  return 0;
}

int skipToFrameAVI2_0(uint32_t framesToSkip, uint32_t * aviMoviListFrameCount) {
  dbgPrint("framesToSkip: " + String(framesToSkip));
  uint32_t moviListStart = infile.curPosition();// + 8;
  uint32_t moviListOffset = 0;
  int baseOffsetToUse = 0;
  while (baseOffsetToUse < aviMoviListCount && framesToSkip > aviMoviListFrameCount[baseOffsetToUse]) {
    framesToSkip -= aviMoviListFrameCount[baseOffsetToUse];
    aviMoviListPos++;
    baseOffsetToUse++;
  }
  // Offset points to ix00 chunk name 4 bytes, first uint32 is length of ix00 chunk, next uint32 & 0x0000FFFF = wLongsPerEntry which should be 2
  // The next uint32 is number of entries, next 4 bytes are id, next uint64 is base offset, next 4 bytes reserved, then index data.
  // Index entries are 32 bit offset in file, 32 bit size
  int timer2 = millis();
  infile.seekSet(aviMoviListOffsets[aviMoviListPos]);
  dbgPrint("seek time " + String(millis() - timer2));
  uint8_t chunkData[32];
  if (infile.read(chunkData, 32) != 32) {
    return 1;
  }
  if (strncmp((char *)chunkData + 0, "ix00", 4) == 0) {
    moviListStart = getIntBE(chunkData + 20);
    dbgPrint("moviListStart = " + String(moviListStart, HEX));
    timer2 = millis();
    infile.seekCur(framesToSkip * 8);
    if (infile.read(chunkData, 4) != 4) {
      return 1;
    }
    dbgPrint("seek time " + String(millis() - timer2));
    moviListOffset = getIntBE(chunkData);
    dbgPrint("moviListOffset = " + String(moviListOffset, HEX));
    dbgPrint("moviListStart + moviListOffset = " + String(moviListStart + moviListOffset, HEX));
    moviListOffset -= 8;//point to chunk header
  }
  infile.seekSet(moviListStart + moviListOffset);
  return 0;
}

int getVideoInfo(int startTimeOffsetS) {
  int chunkCount = 0;
  uint8_t chunkHeader[12];
  currentAudioRate = 0;
  uint32_t frameRate = 0;
  uint32_t totalFrames = 0;
  uint32_t frameWidth = 0;
  uint32_t frameHeight = 0;
  bool isMJPG = false;
  aviMoviListCount = 0;
  aviMoviListPos = 0;
  uint32_t aviMoviListFrameCount[5];
  aviRIFFSize = 0;

  setAVIScreenBuffer();
  #ifndef TinyTVKit
  if(videoBuf[1] == NULL && DOUBLE_BUFFER) {
    videoBuf[1] = (uint8_t*)malloc(VIDEOBUF_SIZE);
  }
  #endif

  if (infile.read(chunkHeader, 12) != 12) {
    return 1;
  }
  // Make sure we can parse it
  if ((strncmp((char *)chunkHeader + 0, "RIFF", 4)) || (strncmp((char *)chunkHeader + 8, "AVI ", 4))) {
    dbgPrint("Infile is not a RIFF AVI file");
    return 1;
  }
  aviRIFFSize = getIntBE(chunkHeader + 4);
  dbgPrint("aviRIFFSize = " + String(aviRIFFSize));

  while (chunkCount < 50) {
    //Read the next chunk/list header
    if (infile.read(chunkHeader, 12) != 12) {
      return 1;
    }

    // get the length of the chunk/list data- we're mostly just skipping the data
    int skipBytes = getIntBE(chunkHeader + 4);

    if (strncmp((char *)chunkHeader + 0, "LIST", 4) == 0) {
      // basically ignore list headers, just look at next chunk inside it
      skipBytes = 0;
    } else {
      // if it's a chunk, we already read the first 4 bytes
      skipBytes -= 4;
    }

    if (strncmp((char *)chunkHeader + 0, "avih", 4) == 0) {
      dbgPrint("Found avih");
      infile.seekCur(28);
      if (infile.read(chunkHeader, 4) != 4) {
        return 1;
      }
      frameWidth = getIntBE(chunkHeader);
      if (infile.read(chunkHeader, 4) != 4) {
        return 1;
      }
      frameHeight = getIntBE(chunkHeader);

      dbgPrint("Width: " + String(frameWidth) + " Height: " + String(frameHeight));
      if (abs((int)VIDEO_W - (int)frameWidth) > 10 || abs((int)VIDEO_H - (int)frameHeight) > 10) {
        dbgPrint("Frame size incorrect!");
        return 1;
      }
      skipBytes -= (28 + 4 + 4);
    }

    if (strncmp((char *)chunkHeader + 0, "strh", 4) == 0) {
      dbgPrint("In stream header");
      if (strncmp((char *)chunkHeader + 8, "auds", 4) == 0) {
        // Found the audio stream header
        dbgPrint("Found audio stream info");
        infile.seekCur(20);
        if (infile.read(chunkHeader, 4) != 4)
        {
          return 1;
        }
        currentAudioRate = getIntBE(chunkHeader);
        if (currentAudioRate) {
          dbgPrint("Set audio rate to " + String(currentAudioRate));
        }
        skipBytes -= 24;
      }
      if (strncmp((char *)chunkHeader + 8, "vids", 4) == 0) {
        // Found the video stream header
        dbgPrint("Found video stream info");
        uint8_t chunkData[32];
        if (infile.read(chunkData, 32) != 32) {
          return 1;
        }
        if (strncmp((char *)chunkData, "MJPG", 4) == 0) {
          isMJPG = true;
        }
        if (strncmp((char *)chunkData, "H264", 4) == 0) {
          /* H264 not implemented for RIFF container */
        }
        if (strncmp((char *)chunkData, "h264", 4) == 0) {
          /* H264 not implemented for RIFF container */
        }
        frameRate = getIntBE(chunkData + 20);
        totalFrames = getIntBE(chunkData + 28);
        skipBytes -= 32;
      }
    }

    if (strncmp((char *)chunkHeader + 0, "indx", 4) == 0) {
      // in indx chunk, first uint32 is length, next uint32 & 0x0000FFFF = wLongsPerEntry should be 4. These are already in chunkHeader
      // Next data is uint32 = number of entries in index, next 32 bits for type(00dc for video), 12 reserved bytes, then index data.
      // Each entry is a uint64 for offset in file(absolute) of chunk, uint32 size, uint32 duration(duration = video 'ticks'? = assumed frames)
      // We're bad and assume the offset fits into uint32_t, we have a 4GB file size limit due to filesystem anyway.
      uint8_t chunkData[20];
      if (infile.read(chunkData, 20) != 20) {
        return 1;
      }
      skipBytes -= 20;
      if (strncmp((char *)chunkData + 4, "00dc", 4) == 0) {
        uint32_t entryCount = getIntBE(chunkData + 0);
        dbgPrint("entryCount = " + String(entryCount));
        dbgPrint("In indx header with 00dc type");
        if (entryCount > 5)// This should be a safe assumtion with 4GB max file size.
          entryCount = 5;
        for (int i = 0; i < entryCount; i++) {
          if (infile.read(chunkData, 16) != 16) {
            return 1;
          }
          skipBytes -= 16;
          dbgPrint("offset = " + String(getIntBE(chunkData + 0), HEX) + " duration = " + String(getIntBE(chunkData + 12)));
          aviMoviListOffsets[aviMoviListCount] = getIntBE(chunkData + 0);
          aviMoviListFrameCount[aviMoviListCount] = getIntBE(chunkData + 12);
          aviMoviListCount++;
        }
      }
    }

    if (strncmp((char *)chunkHeader + 8, "movi", 4) == 0) {
      dbgPrint("Found movi list, ready to stream?");
      skipBytes = getIntBE(chunkHeader + 4);
      dbgPrint(String(skipBytes));
      dbgPrint(String("sizeof: ") + String(sizeof(infile)));
      int framesToSkip = (startTimeOffsetS * frameRate) % totalFrames;
      if (framesToSkip > 0) {
        int timer = millis();
        if (aviMoviListCount) { // AVI 2.0
          skipToFrameAVI2_0(framesToSkip, aviMoviListFrameCount);
          dbgPrint("skip time " + String(millis() - timer));
        } else {// Use legacy index
          skipToFrameAVI1_0(skipBytes, framesToSkip);
        }
        dbgPrint("skip time " + String(millis() - timer));
      }

      if ((isMJPG) && frameRate) {
        if (isMJPG) {
          videoFormat = 0;
        }
        targetFrameTime = 1000000 / frameRate;
        dbgPrint("Set target frametime to " + String((uint32_t)targetFrameTime));
        if (infile.read(nextChunkTag, 8) != 8)
        {
          return 1;
        }
        videoStreamReadyAVI = true;
        return 0;
      }
      return 1;
    }

    if ((skipBytes & 1) != 0) skipBytes++; // padding
    infile.seekCur(skipBytes);
    chunkCount++;
  }
  return 1;
}

int readTSVBytes(uint8_t * dest, int maxLen) {
  return infile.read(dest, maxLen);
}

int getTSVVideoInfo(int startTimeOffsetS) {
  
  uint32_t frameRate = 30;
  currentAudioRate = frameRate * 1024;
  targetFrameTime = 1000000 / frameRate;
  videoStreamReadyTSV = true;

  uint32_t frameSizeBytes = (VIDEO_W * VIDEO_H * 2) + (1024 * 2); // video + audio bytes per frame
  uint32_t totalFrames = infile.fileSize() / frameSizeBytes;

  int framesToSkip = startTimeOffsetS * frameRate;
  framesToSkip = framesToSkip % totalFrames;

  infile.seekSet(framesToSkip * frameSizeBytes);
  return 0;
}

bool isAVIStreamAvailable() {
  return videoStreamReadyAVI;
}

bool isTSVStreamAvailable() {
  return videoStreamReadyTSV;
}

bool isMP4StreamAvailable() {
  return videoStreamReadyMP4;
}

int getVideoAudioRate() {
  return currentAudioRate;
}

int getVideoFormat() {
  return videoFormat;
}

int startVideo(const char* n, int startTimeS) {
  delay(200);
  firstFrame = true;
  videoFormat = 0;

  dbgPrint("Starting "+String(n));
  
  videoStreamReadyAVI = false;
  videoStreamReadyTSV = false;
  videoStreamReadyMP4 = false;

#ifndef TinyTVKit

while (!display.getReadyStatusDMA()) {}

  while(getFilledJPEGBuffer()) {yield();} // Wait for core2 to finish so we don't yank the decoder out from under it
  
  if(isMP4StreamAvailable()) while(getH264DecodeReady()) {yield();} // Wait for core2 to finish so we don't yank the decoder out from under it

  currentFLACDataChunk = numFLACDataChunks = 0;
  
  if (decHd && wasInit) {
    dbgPrint("h264 shutdown");
    h264bsdShutdown(decHd);
    wasInit = false;
  }
#endif

  if (n[0] != 0) {
    // Open the video file or the next one if we can't
    infile.close();
    if (!infile.open(n, O_RDONLY)) {
      dbgPrint("Video open error");
      sd.card()->syncDevice();
      sd.cacheClear();
      return 1;
    }
  }
  infile.rewind();
  dbgPrint("Cleared space for decoder...");

  char fileName[13];
  memset(fileName, 0, 13);
  infile.getSFN(fileName, 13);
  if (!strcmp(fileName + strlen(fileName) - 4, ".avi") || !strcmp(fileName + strlen(fileName) - 4, ".AVI")) {
    setAVIScreenBuffer();
    if (getVideoInfo(startTimeS)) {
      dbgPrint("Error finding stream info or read error");
      return 3;
    }
  } else if (!strcmp(fileName + strlen(fileName) - 4, ".tsv") || !strcmp(fileName + strlen(fileName) - 4, ".TSV")) {
    setAVIScreenBuffer();
    getTSVVideoInfo(startTimeS);
  } 
  #ifndef TinyTVKit
  else if (!strcmp(fileName + strlen(fileName) - 4, ".mp4") || !strcmp(fileName + strlen(fileName) - 4, ".MP4")) {

    setMP4ScreenBuffer();
    
    if (h264bsdInit(decHd, false) == HANTRO_OK) {
      dbgPrint("SUCCESS: Initialzed MP4/H264 decoder!\n");
      wasInit = true;
    } else {
      dbgPrint("ERROR: Could not Initialize MP4/H264 decoder!\n");
    }

    fx_flac_reset(decFlac);
    
    getMP4VideoInfo(startTimeS, decHd, decFlac);
    dbgPrint("Got MP4 info");
  } 
  #endif 
  else {
    #ifdef TinyTVKit
    dbgPrint("Error- file is not AVI or TSV");
    #else
    dbgPrint("Error- file is not AVI, TSV or MP4");
    #endif
    return 4;
  }
  tsMillisInitial = millis();
  return 0;
}

char * getCurrentFilename() {
  return aviList[channelNumber - 1];
}

int startVideoByChannel(int channelNum) {
  channelNumber = channelNum;
  if (channelNumber < 1) {
    channelNumber = 1;
  }
  if (channelNumber > aviCount) {
    channelNumber = aviCount;
  }

  dbgPrint("Playing " + String(aviList[channelNumber - 1]) + " Channel # is " + String(channelNumber));

  if (startVideo(aviList[channelNumber - 1], liveMode ? (millis() + millisOffset) / 1000 : 0)) {
    return 1;
  }
  return 0;
}

int prevVideo() {
  channelNumber--;
  if (channelNumber < 1) {
    channelNumber = aviCount;
  }
  return startVideoByChannel(channelNumber);
}

int nextVideo() {
  channelNumber++;
  if (channelNumber > aviCount) {
    channelNumber = 1;
  }
  return startVideoByChannel(channelNumber);
}

// Replace strcasecmp with natural sorting algorithm from:
// https://stackoverflow.com/questions/13856975/how-to-sort-file-names-with-numbers-and-alphabets-in-order-in-c
int strcasecmp_withNumbers(const void *void_a, const void *void_b) {
  const char *a = (char const *)void_a;
  const char *b = (char const *)void_b;

  if (!a || !b) { // if one doesn't exist, other wins by default
    return a ? 1 : b ? -1 : 0;
  }
  if (isdigit(*a) && isdigit(*b)) { // if both start with numbers
    char *remainderA;
    char *remainderB;
    long valA = strtol(a, &remainderA, 10);
    long valB = strtol(b, &remainderB, 10);
    if (valA != valB)
      return valA - valB;
    // if you wish 7 == 007, comment out the next two lines
    else if (remainderB - b != remainderA - a) // equal with diff lengths
      return (remainderB - b) - (remainderA - a); // set 007 before 7
    else // if numerical parts equal, recurse
      return strcasecmp_withNumbers(remainderA, remainderB);
  }
  if (isdigit(*a) || isdigit(*b)) { // if just one is a number
    return isdigit(*a) ? -1 : 1; // numbers always come first
  }
  while (*a && *b) { // non-numeric characters
    if (isdigit(*a) || isdigit(*b))
      return strcasecmp_withNumbers(a, b); // recurse
    if (tolower(*a) != tolower(*b))
      return tolower(*a) - tolower(*b);
    a++;
    b++;
  }
  
  return *a ? 1 : *b ? -1 : 0;
}

int loadVideoList(char * splashFN) {
  char * temporaryFileNameList = (char *)sharedBuffer; //use video and audio buffers to alphabetize filenames
  int tempFileNameLength = MAX_LFN_LEN;
  infile.close();
  File32 rootDir;
  if (!rootDir.openRoot(sd.vol())) {
    dbgPrint("SD read error?");
  }
  char fileName[MAX_LFN_LEN];
  aviCount = 0;
  while (infile.openNext(&rootDir, O_RDONLY) && aviCount < maxVideos) {
    memset(fileName, 0, MAX_LFN_LEN);
    infile.getSFN(fileName, MAX_LFN_LEN);
    infile.getName(fileName, MAX_LFN_LEN);
    if (fileName[0] != '.') {

      if ( !strcasecmp(fileName, "splash.avi") || !strcasecmp(fileName, "splash.tsv") || !strcasecmp(fileName, "splash.mp4") ) {
        // Do not add to list
        infile.getSFN(splashFN, 15);
      } else if (
        !strcasecmp(fileName + strlen(fileName) - 4, ".avi") 
        || !strcasecmp(fileName + strlen(fileName) - 4, ".tsv") 
        || !strcasecmp(fileName + strlen(fileName) - 4, ".mp4")
      ) {
        strcpy(temporaryFileNameList + (aviCount * tempFileNameLength), fileName);
        aviCount++;
      } else {
        // Unknown file extension
      }
      
    }
    int e =  rootDir.getError();
    if (e) {
      dbgPrint("Directory error " + String(e));
      break;
    }
    infile.close();
  }
  dbgPrint("");
  for (int i = 0; i < aviCount; i++) {
    dbgPrint(temporaryFileNameList + (i * tempFileNameLength));
  }
  dbgPrint("");
  if (alphabetizedPlaylist) {
    qsort(temporaryFileNameList, aviCount, tempFileNameLength, strcasecmp_withNumbers);
    for (int i = 0; i < aviCount; i++) {
      dbgPrint(temporaryFileNameList + (i * tempFileNameLength));
    }
    dbgPrint("");
  }
  rootDir.close();
  for (int i = 0; i < aviCount; i++) {
    if (infile.open(temporaryFileNameList + (i * tempFileNameLength), O_RDONLY)) {
      infile.getSFN(aviList[i], 13);
    }
    infile.close();
  }
  
  dbgPrint("");
  for (int i = 0; i < aviCount; i++) {
    dbgPrint(aviList[i]);
  }
  dbgPrint("");
  return aviCount;
}

// Before USB MSC serves raw sectors to the host, close all Fat32 file handles and flush caches.
// Leaving infile open during playback caused raw readSectors() to fight SdFat's volume cache; macOS
// then failed to mount (Disk Utility: Not Mounted, 0 B) while Windows often still worked.
void releaseSdCardForUSBMSC() {
#ifndef TinyTVKit
  while (!display.getReadyStatusDMA()) {
#ifdef ARDUINO_ARCH_RP2040
    msc_yield_usb_only();
#else
    yield();
#endif
  }
  while (getFilledJPEGBuffer()) {
#ifdef ARDUINO_ARCH_RP2040
    msc_yield_usb_only();
#else
    yield();
#endif
  }
  if (isMP4StreamAvailable()) {
    while (getH264DecodeReady()) {
#ifdef ARDUINO_ARCH_RP2040
      msc_yield_usb_only();
#else
      yield();
#endif
    }
  }
#endif
  infile.close();
  sd.card()->syncDevice();
  sd.cacheClear();
}
