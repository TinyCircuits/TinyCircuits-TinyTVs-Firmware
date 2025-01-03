//-------------------------------------------------------------------------------
//  TinyCircuits TinyTV Firmware
//
//  Changelog:
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
uint64_t tsMillisInitial = 0;
uint32_t currentAudioRate = 0;
uint64_t millisOffset = 0;
uint32_t aviRIFFSize = 0;
int aviMoviListCount = 0;
int aviMoviListPos = 0;
uint32_t aviMoviListOffsets[5];

void setMillisOffset(uint64_t val) {
  millisOffset = val;
}
uint64_t getMillisOffset() {
  return millisOffset;
}

uint32_t getInt(uint8_t * intOffset) {
  return (uint32_t(intOffset[3]) << 24) | (uint32_t(intOffset[2]) << 16) | (uint32_t(intOffset[1]) << 8) | (uint32_t(intOffset[0]));
}

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
  return getInt(nextChunkTag + 4);
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
  /*// the hard way..
    aviMoviListPos++;
    if (aviMoviListPos < aviMoviListCount) {
    uint32_t framesToSkip = 0;
    uint32_t moviListStart = 0;
    uint32_t moviListOffset = 0;
    // Offset points to ix00 chunk name 4 bytes, first uint32 is length of ix00 chunk, next uint32 & 0x0000FFFF = wLongsPerEntry which should be 2
    // The next uint32 is number of entries, next 4 bytes are id, next uint64 is base offset, next 4 bytes reserved, then index data.
    // Index entries are 32 bit offset in file, 32 bit size
    infile.seekSet(aviMoviListOffsets[aviMoviListPos]);
    uint8_t chunkData[32];
    if (infile.read(chunkData, 32) != 32) {
      return 1;
    }
    if (strncmp((char *)chunkData + 0, "ix00", 4) == 0) {
      moviListStart = getInt(chunkData + 20);
      dbgPrint("moviListStart = " + String(moviListStart, HEX));
      infile.seekCur(framesToSkip * 8);
      if (infile.read(chunkData, 4) != 4) {
        return 1;
      }
      moviListOffset = getInt(chunkData);
      dbgPrint("moviListOffset = " + String(moviListOffset, HEX));
      dbgPrint("moviListStart + moviListOffset = " + String(moviListStart + moviListOffset, HEX));
      moviListOffset -= 8;//point to chunk header
    }
    int timer2 = millis();
    infile.seekSet(moviListStart + moviListOffset);
    infile.read(nextChunkTag, 8);
    dbgPrint("seek time " + String(millis() - timer2));
    }*/
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
    //skipBytes = getInt(chunkHeader + 4);
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
          moviListOffset = getInt(tempBuf + bufPos + 8);
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
    moviListStart = getInt(chunkData + 20);
    dbgPrint("moviListStart = " + String(moviListStart, HEX));
    timer2 = millis();
    infile.seekCur(framesToSkip * 8);
    if (infile.read(chunkData, 4) != 4) {
      return 1;
    }
    dbgPrint("seek time " + String(millis() - timer2));
    moviListOffset = getInt(chunkData);
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

  if (infile.read(chunkHeader, 12) != 12) {
    return 1;
  }
  // Make sure we can parse it
  if ((strncmp((char *)chunkHeader + 0, "RIFF", 4)) || (strncmp((char *)chunkHeader + 8, "AVI ", 4))) {
    dbgPrint("Infile is not a RIFF AVI file");
    return 1;
  }
  aviRIFFSize = getInt(chunkHeader + 4);
  dbgPrint("aviRIFFSize = " + String(aviRIFFSize));

  while (chunkCount < 50) {
    //Read the next chunk/list header
    if (infile.read(chunkHeader, 12) != 12) {
      return 1;
    }

    // get the length of the chunk/list data- we're mostly just skipping the data
    int skipBytes = getInt(chunkHeader + 4);

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
      if (infile.read(chunkHeader, 4) != 4){
        return 1;
      }
      frameWidth = getInt(chunkHeader);
      if (infile.read(chunkHeader, 4) != 4){
        return 1;
      }
      frameHeight = getInt(chunkHeader);
      
      dbgPrint("Width: " + String(frameWidth)+ " Height: " + String(frameHeight));
      if (abs((int)VIDEO_W-(int)frameWidth)>10 || abs((int)VIDEO_H-(int)frameHeight)>10) {
        dbgPrint("Frame size incorrect!");
        return 1;
      }
      skipBytes -= (28+4+4);
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
        currentAudioRate = getInt(chunkHeader);
        if (currentAudioRate) {
          //samplesPerFrame =  audioRate / (1000000 / targetFrameTime) + 1;
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
        frameRate = getInt(chunkData + 20);
        totalFrames = getInt(chunkData + 28);
        skipBytes -= 32;
      }
    }

    if (strncmp((char *)chunkHeader + 0, "indx", 4) == 0) {
      // in indx chunk, first uint32 is length, next uint32 & 0x0000FFFF = wLongsPerEntry should be 4. These are already in chunkHeader
      // Next data is uint32 = number of entries in index, next 32 bits for type(00dc for video), 12 reserved bytes, then index data.
      // Each entry is a uint64 for offset in file(absolute) of chunk, uint32 size, uint32 duration(duration = video 'ticks'? = assumed frames)
      // We're bad and assume the offset fits into uint32_t, we have a 4GB file size limit due to filesystem anyway.
      const int bytes = 4 + 4 + 12 + (5 * 16);
      uint8_t chunkData[20];
      if (infile.read(chunkData, 20) != 20) {
        return 1;
      }
      skipBytes -= 20;
      if (strncmp((char *)chunkData + 4, "00dc", 4) == 0) {
        uint32_t entryCount = getInt(chunkData + 0);
        dbgPrint("entryCount = " + String(entryCount));
        dbgPrint("In indx header with 00dc type");
        if (entryCount > 5)// This should be a safe assumtion with 4GB max file size.
          entryCount = 5;
        for (int i = 0; i < entryCount; i++) {
          if (infile.read(chunkData, 16) != 16) {
            return 1;
          }
          skipBytes -= 16;
          dbgPrint("offset = " + String(getInt(chunkData + 0), HEX) + " duration = " + String(getInt(chunkData + 12)));
          aviMoviListOffsets[aviMoviListCount] = getInt(chunkData + 0);
          aviMoviListFrameCount[aviMoviListCount] = getInt(chunkData + 12);
          aviMoviListCount++;
        }
      }
    }

    if (strncmp((char *)chunkHeader + 8, "movi", 4) == 0) {
      dbgPrint("Found movi list, ready to stream?");
      skipBytes = getInt(chunkHeader + 4);
      dbgPrint(String(skipBytes));
      dbgPrint(String("sizeof: ") + String(sizeof(infile)));
      int framesToSkip = (startTimeOffsetS * frameRate) % totalFrames;
      if (framesToSkip > 0) {
        int timer = millis();
        //aviMoviListCount = 0;
        if (aviMoviListCount) { // AVI 2.0
          skipToFrameAVI2_0(framesToSkip, aviMoviListFrameCount);
          dbgPrint("skip time " + String(millis() - timer));
        } else {// Use legacy index
          skipToFrameAVI1_0(skipBytes, framesToSkip);
        }
        dbgPrint("skip time " + String(millis() - timer));
      }

      if (isMJPG && frameRate) {
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

int getVideoAudioRate() {
  return currentAudioRate;
}

int startVideo(const char* n, int startTimeS) {
  videoStreamReadyAVI = false;
  videoStreamReadyTSV = false;

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


  char fileName[13];
  memset(fileName, 0, 13);
  infile.getSFN(fileName, 13);
  if (!strcmp(fileName + strlen(fileName) - 4, ".avi") || !strcmp(fileName + strlen(fileName) - 4, ".AVI")) {
    if (getVideoInfo(startTimeS)) {
      dbgPrint("Error finding stream info or read error");
      return 3;
    }
  } else if (!strcmp(fileName + strlen(fileName) - 4, ".tsv") || !strcmp(fileName + strlen(fileName) - 4, ".TSV")) {
    getTSVVideoInfo(startTimeS);
  } else {
    dbgPrint("Error- file is not AVI or TSV");
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

//#define cdc SerialUSB
int loadVideoList() {
  //delay(5000);
  char * temporaryFileNameList = (char *)sharedBuffer; //use video and audio buffers to alphabetize filenames
  int tempFileNameLength = MAX_LFN_LEN;
#ifndef TinyTVKit
  //sd.vol()->ls(&);
#else
  //sd.vol()->ls(&Serial);
#endif
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
      if (!strcasecmp(fileName + strlen(fileName) - 4, ".avi") || !strcasecmp(fileName + strlen(fileName) - 4, ".tsv")) {
        //strcpy(aviList[aviCount], fileName);
        strcpy(temporaryFileNameList + (aviCount * tempFileNameLength), fileName);
        aviCount++;
      } else {
        //dbgPrint("Found non-AVI file, skipping");
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
    //dbgPrint(aviList[i]);
    dbgPrint(temporaryFileNameList + (i * tempFileNameLength));
  }
  dbgPrint("");
  if (alphabetizedPlaylist) {
    qsort(temporaryFileNameList, aviCount, tempFileNameLength, strcasecmp_withNumbers);
    for (int i = 0; i < aviCount; i++) {
      //dbgPrint(aviList[i]);
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
