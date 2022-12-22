//-------------------------------------------------------------------------------
//  TinyCircuits RP2040TV Video Player, Initialization Component
//
//  Changelog:
//  08/12/2022 Handed off the keys to the kingdom
//
//  Written by Mason Watmough for TinyCircuits, http://TinyCircuits.com
//
//-------------------------------------------------------------------------------

/*
    This file is part of the RP2040TV Player.
    RP2040TV Player is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    RP2040TV Player is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with
    the RP2040TV Player. If not, see <https://www.gnu.org/licenses/>.
*/

// FFMPEG VIDEO ENCODE COMMANDS:
// 30fps:
//    ffmpeg -i "videofile.mp4" -r 30 -vf "scale=216:135,hqdn3d" -b:v 800k -maxrate 800k -bufsize 48k -c:v mjpeg -acodec pcm_u8 -ar 10000 -ac 1 in.avi
// 24fps:
//    ffmpeg -i "videofile.mp4" -r 24 -vf "scale=216:135,hqdn3d" -b:v 1500k -maxrate 1500k -bufsize 64k -c:v mjpeg -acodec pcm_u8 -ar 10000 -ac 1 in.avi


const int SEEKBUF_LEN = 4096;
char seekBuf[SEEKBUF_LEN];

char aviList[50][50] = {"\0"};
int aviCount = 0;
uint32_t livePos;
uint8_t nextChunkTag[8];
bool videoStreamReady = false;

void saveSettings()
{
  uint8_t tempBuff[500];
  File32 settingsFile;
  settingsFile.open("settings.txt", O_WRITE | O_CREAT);
  sprintf((char*)tempBuff, "Volume: %hi\n\rStatic vfx/sfx: %hhi\n\rPlayback mode: %hhi\n\rShow timestamp: %hhi\n\rDisplay channel #: %hhi\n\rAlphabetize playback: %hhi", soundVolume, doStaticEffects, autoplayMode, timeStamp, showChannelNumber, alphabetizedPlaylist);
  settingsFile.write((const char*)tempBuff);
  settingsFile.close();
}

uint32_t getInt(uint8_t * intOffset) {
  return (uint32_t(intOffset[3]) << 24) | (uint32_t(intOffset[2]) << 16) | (uint32_t(intOffset[1]) << 8) | (uint32_t(intOffset[0]));
}

int readNextChunk(uint8_t * dest, int maxLen) {
  //  int chunkLen = nextChunkLength();
  //  if ((chunkLen & 1) != 0) { //padding
  //    chunkLen+=1;
  //  }
  //  if (infile.read(dest, min(chunkLen + 8, maxLen)) == (chunkLen + 8)) {
  //    memcpy(nextChunkTag, dest + nextChunkLength(), 8);
  //    return 0;
  //  }
  //  return 1;

  int chunkLen = nextChunkLength();
  int chunkRead = infile.read(dest, min(chunkLen, maxLen));


  if (chunkLen != chunkRead) {
    cdc.println("chunkskip ?" );
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

int getVideoInfo() {
  int chunkCount = 0;
  uint8_t chunkHeader[12];
  uint32_t audioRate = 0;
  uint32_t frameRate = 0;
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
        audioRate = getInt(chunkHeader);
        skipBytes -= 24;
      }
      if (strncmp((char *)chunkHeader + 8, "vids", 4) == 0) {
        // Found the video stream header
        dbgPrint("Found video stream info");
        uint8_t chunkData[24];
        if (infile.read(chunkData, 24) != 24)
        {
          return 1;
        }
        if (strncmp((char *)chunkData, "MJPG", 4) == 0) {
          isMJPG = true;
        }
        frameRate = getInt(chunkData + 20);
        skipBytes -= 24;
      }
    }

    if (strncmp((char *)chunkHeader + 8, "movi", 4) == 0) {
      // Start loading AV data
      dbgPrint("Found movi list, ready to stream?");
      if (isMJPG && frameRate) {
        targetFrameTime = 1000000 / frameRate;
        dbgPrint("Set target frametime to " + String((uint32_t)targetFrameTime));
        if (audioRate) {
          //samplesPerFrame =  audioRate / (1000000 / targetFrameTime) + 1;
          dbgPrint("Set audio rate to " + String(audioRate));
          setAudioSampleRate(audioRate);
        }
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

int startVideo(const char* n) {
  videoStreamReady = false;
  if (n[0] != 0) {
    // Open the video file or the next one if we can't
    infile.close();
    if (!infile.open(n, O_RDONLY)) {
      dbgPrint("Video open error");
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
  if (getVideoInfo()) {
    dbgPrint("Error finding stream info or read error");
    return 3;
  }
  tsMillisInitial = millis();
  return 0;
}


char * getCurrentFilename() {
  return aviList[channelNumber - 1];
}



int prevVideo() {
  int currentVideo = channelNumber - 1;
  currentVideo--;
  if (currentVideo < 0) {
    currentVideo = aviCount - 1;
  }
  dbgPrint("Playing " + String(aviList[currentVideo]) + " Channel # is " + String(channelNumber));

  channelNumber = currentVideo + 1;
  showChannelTimer = 120;
  playWhiteNoise = false;

  if (startVideo(aviList[currentVideo])) {
    return 1;
  }
  return 0;
}

int nextVideo() {
  int currentVideo = channelNumber - 1;
  currentVideo++;
  if (currentVideo >= aviCount) {
    currentVideo = 0;
  }

  dbgPrint("Playing " + String(aviList[currentVideo]) + " Channel # is " + String(channelNumber));

  channelNumber = currentVideo + 1;
  showChannelTimer = 120;
  playWhiteNoise = false;

  if (startVideo(aviList[currentVideo])) {
    return 1;
  }
  return 0;
}

void loadSettings() {
  // Load the user settings from settings.txt
  File32 settingsFile;
  if (!settingsFile.open("settings.txt", O_READ)) {
    settingsFile.close();
    saveSettings();
  } else {
    uint8_t tempBuff[500];
    settingsFile.read(tempBuff, sizeof(tempBuff));
    sscanf((const char*)tempBuff, "Volume: %hi\n\rStatic vfx/sfx: %hhi\n\rPlayback mode: %hhi\n\rShow timestamp: %hhi\n\rDisplay channel #: %hhi\n\rAlphabetize playback: %hhi", &soundVolume, &doStaticEffects, &autoplayMode, &timeStamp, &showChannelNumber, &alphabetizedPlaylist);
    settingsFile.close();
  }
}

int cmpstr(void const *a, void const *b) {
  char const *aa = (char const *)a;
  char const *bb = (char const *)b;

  return strcasecmp(aa, bb);
}

int loadVideoList() {
  infile.close();
  if (!dir.open("/", O_RDONLY)) {
    cdc.println("SD read error?");
  }
  dir.rewind();
  char fileName[100];
  aviCount = 0;
  while (infile.openNext(&dir, O_RDONLY)) {
    memset(fileName, 0, 100);
    infile.getName(fileName, 100);
    if (fileName[0] != '.') {
      if (!strcmp(fileName + strlen(fileName) - 4, ".avi")) {
        strcpy(aviList[aviCount], fileName);
        aviCount++;
      }
    }
    infile.close();
  }
  int count = aviCount;
  for (int i = 0; i < aviCount; i++) {
    cdc.println(aviList[i]);
  }

  if (alphabetizedPlaylist) {
    qsort(aviList, aviCount, sizeof(aviList[0]), cmpstr);
    for (int i = 0; i < aviCount; i++) {
      cdc.println(aviList[i]);
    }
  }
  dir.close();
  return count;
}
