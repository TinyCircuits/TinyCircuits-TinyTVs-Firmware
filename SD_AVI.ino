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
bool videoStreamReady = false;
uint64_t tsMillisInitial = 0;
uint32_t currentAudioRate = 0;

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

int nextChunkLength() {
  return getInt(nextChunkTag + 4);
}

int getVideoInfo(int startTimeOffsetS) {
  int chunkCount = 0;
  uint8_t chunkHeader[12];
  currentAudioRate = 0;
  uint32_t frameRate = 0;
  uint32_t totalFrames = 0;
  bool isMJPG = false;
  while (chunkCount < 50) {
    //Read the next chunk/list header
    if (infile.read(chunkHeader, 12) != 12)
    {
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

    if (strncmp((char *)chunkHeader + 8, "movi", 4) == 0) {
      dbgPrint("Found movi list, ready to stream?");
      skipBytes = getInt(chunkHeader + 4);
      dbgPrint(String(skipBytes));
      int moviListStart = infile.curPosition();// + 8;
      int moviListOffset = 0;
      int timer = millis();
      if (startTimeOffsetS > 0) {
        infile.seekCur(skipBytes - 4);
        if (infile.read(chunkHeader, 8) != 8) {
          return 1;
        }
        if (strncmp((char *)chunkHeader, "idx1", 4) == 0) {
          skipBytes = getInt(chunkHeader + 4);
          dbgPrint("Found idx1, size: " + String(skipBytes));
          int framesToSkip = startTimeOffsetS * frameRate;
          framesToSkip = framesToSkip % totalFrames;
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
          //need to test this..
          //          while (KBtoSkip * 1024 > skipBytes - 1024) {
          //            KBtoSKip--;
          //          }
          dbgPrint(String(framesPerKB));
          dbgPrint(String(KBtoSkip));
          infile.seekCur(KBtoSkip * 1024);
          framesToSkip -= (KBtoSkip * framesPerKB);
          if (framesToSkip < 1)framesToSkip = 1;

          //continue with 'exact' frame counting
          while (framesToSkip) {
            bufPos = 0;
            int bytesRead = infile.read(tempBuf, 512);
            if (bytesRead != 512) {
              dbgPrint("error reading idx1");
              //don't error out, just use last found moviListOffset?
              //return 1;
              framesToSkip = 0;
            }
            while (framesToSkip && bufPos < bytesRead) {
              if (strncmp((char *)tempBuf + bufPos, "00dc", 4) == 0) {
                framesToSkip--;
                moviListOffset = getInt(tempBuf + bufPos + 8);
              }
              bufPos += 16;
            }
            dbgPrint(String(framesToSkip));
          }

          dbgPrint("skip time " + String(millis() - timer));
          moviListOffset -= 4; //moviListStart already has first tag
        } else {
          dbgPrint("idx1 not at expected position");
        }
        infile.seekSet(moviListStart + moviListOffset);
      }

      if (isMJPG && frameRate) {
        targetFrameTime = 1000000 / frameRate;
        dbgPrint("Set target frametime to " + String((uint32_t)targetFrameTime));
        if (infile.read(nextChunkTag, 8) != 8)
        {
          return 1;
        }
        videoStreamReady = true;
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

bool isAVIStreamAvailable() {
  return videoStreamReady;
}

int getVideoAudioRate() {
  return currentAudioRate;
}

int startVideo(const char* n, int startTimeS) {
  videoStreamReady = false;
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
  char fileHeader[12] = {0};
  infile.rewind();
  if (infile.read(fileHeader, 12) != 12) {
    dbgPrint("Video read error");
    return 1;
  }
  // Make sure we can parse it
  if ((strncmp((char *)fileHeader + 0, "RIFF", 4)) || (strncmp((char *)fileHeader + 8, "AVI ", 4))) {
    dbgPrint("Infile is not a RIFF AVI file");
    return 2;
  }
  if (getVideoInfo(startTimeS)) {
    dbgPrint("Error finding stream info or read error");
    return 3;
  }
  tsMillisInitial = millis();
  return 0;
}


char * getCurrentFilename() {
  return aviList[channelNumber];
}

int startVideoByChannel(int channelNum) {
  int newVideoIndex = channelNum;
  if (newVideoIndex >= aviCount) {
    newVideoIndex = aviCount - 1;
  }

  dbgPrint("Playing " + String(aviList[newVideoIndex]) + " Channel # is " + String(newVideoIndex));

  channelNumber = newVideoIndex;

  if (startVideo(aviList[newVideoIndex], liveMode ? millis() / 1000 : 0)) {
    return 1;
  }
  return 0;
}

int prevVideo() {
  int currentVideo = channelNumber;
  currentVideo--;
  if (currentVideo < 0) {
    currentVideo = aviCount - 1;
  }
  dbgPrint("Playing " + String(aviList[currentVideo]) + " Channel # is " + String(channelNumber));

  channelNumber = currentVideo;

  if (startVideo(aviList[currentVideo], liveMode ? millis() / 1000 : 0)) {
    return 1;
  }
  return 0;
}

int nextVideo() {
  int currentVideo = channelNumber ;
  currentVideo++;
  if (currentVideo >= aviCount) {
    currentVideo = 0;
  }

  dbgPrint("Playing " + String(aviList[currentVideo]) + " Channel # is " + String(channelNumber));

  channelNumber = currentVideo;

  if (startVideo(aviList[currentVideo], liveMode ? millis() / 1000 : 0)) {
    return 1;
  }
  return 0;
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
      if (!strcmp(fileName + strlen(fileName) - 4, ".avi")) {
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
