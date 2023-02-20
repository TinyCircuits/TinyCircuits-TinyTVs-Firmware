//-------------------------------------------------------------------------------
//  TinyCircuits RP2040TV Video Player, USB Callbacks Component
//
//  Changelog:
//  08/12/2022 Handed off the keys to the kingdom
//  
//  02/08/2023 Cross-platform base committed
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


bool ejected = false;
bool mscStart = false;
bool fs_flushed = false;
bool mscActive = false;

volatile uint32_t lbaToRead = 0;
volatile uint8_t* lbaToReadPos = NULL;
volatile uint32_t lbaToReadCount = 0;
volatile uint32_t lbaToWrite = 0;
volatile uint8_t lbaToWritePos = 0;
volatile uint32_t lbaToWriteCount = 0;
uint8_t lbaWriteBuff[512 * 8];

uint32_t sectorLBACount = 1;

const bool secondCoreSD = false;

int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize)
{
  if (secondCoreSD && !ejected ) {
    while (lbaToWriteCount || lbaToReadCount);
    volatile int count = bufsize / 512;
    lbaToRead = lba * sectorLBACount;
    lbaToReadPos = (uint8_t*)buffer;
    lbaToReadCount = count;
    while (lbaToReadCount == count) {};
    return bufsize;
  } else if(!ejected) {
    return sd.card()->readSectors(lba * sectorLBACount, (uint8_t *)buffer, bufsize / 512) ? bufsize : -1;
  }
  return 0;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
  if (secondCoreSD && !ejected ) {
    while (lbaToWriteCount || lbaToReadCount);
    memcpy(lbaWriteBuff, buffer, bufsize);
    int count = bufsize / 512;
    lbaToWrite = lba * sectorLBACount;
    lbaToWritePos = 0;
    lbaToWriteCount = count;
    return bufsize;
  } else if(!ejected) {
    return sd.card()->writeSectors(lba * sectorLBACount, buffer, bufsize / 512) ? bufsize : -1;
  }
  return 0;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb(void)
{
  if (secondCoreSD && !ejected ) {
    fs_flushed = true;
  } else if(!ejected) {
    sd.card()->syncDevice();
    sd.cacheClear();
  }
}

// This callback is a C symbol because our Adafruit USB version doesn't expose a function pointer to it

#ifdef __cplusplus
extern "C" {
#endif
extern void displayNoVideosFound();
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
  (void) lun;
  (void) power_condition;
  if (load_eject)
  {
    if (start)
    {
      mscStart = true;
    }
    else
    {
      ejected = true;
    }
  }
  return true;
}
#ifdef __cplusplus
}
#endif

void USBMSCInit() {
  usb_msc.setID("TinyCircuits", "RP2040TV", "1.0");
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
  usb_msc.setUnitReady(false);
  usb_msc.begin();
}
void USBMSCReady() {
  uint32_t block_count = sd.card()->sectorCount();
  usb_msc.setCapacity(block_count, 512);
}

bool lastState = false;

int count = 0;
int timer = 0;
bool ended = false;


bool USBJustConnected() {
  if (tud_connected()) {
    if (lastState == false && ejected == false) {
      lastState = true;
      return true;
    }
  } else {
    lastState = false;
    ejected = false;
  }
  return false;
}

void USBMSCStart() {
  count = 0;
  timer = millis();
  mscActive = true;
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
  usb_msc.setUnitReady(true);
}
bool USBMSCJustStopped() {
  if (ended) {
    ended = false;
    return true;
  }
  return false;
}
bool handleUSBMSC(bool stopMSC) {
  if (mscActive) {
    if (count < 100 && !stopMSC && !ejected) {
      if ((millis() - timer > 1000) && !tud_ready() ) {
        count++;
        delay(1);
      } else {
        count = 0;
      }
      yield();
      return true;
    }


    for (int i = 0; i < 50; i++) {
      delay(1);
      yield();
    }
    usb_msc.setUnitReady(false);
    for (int i = 0; i < 50; i++) {
      delay(1);
      yield();
    }
    mscActive = false;
    dbgPrint("Media ejected.");
    sd.card()->syncDevice();
    sd.cacheClear();
    while(sd.card()->isBusy()) {}
    usb_msc.setReadWriteCallback(nullptr, nullptr, nullptr);
    ended = true;
  }
  return false;
}


void MSCloopCore1() {
  if (secondCoreSD && !ejected ) {
    if (lbaToReadCount) {
      sd.card()->readSectors(lbaToRead, (uint8_t*) lbaToReadPos, 1);
      lbaToReadPos += 512;
      lbaToRead += 1;
      lbaToReadCount -= 1;
      //read first block, then remainder
      if (lbaToReadCount) {
        sd.card()->readSectors(lbaToRead, (uint8_t*) lbaToReadPos, lbaToReadCount);
      }
      lbaToReadCount = 0;
    }
    if (lbaToWriteCount) {
      sd.card()->writeSectors(lbaToWrite, (uint8_t*)lbaWriteBuff + lbaToWritePos, lbaToWriteCount);
      lbaToWriteCount = 0;
    }
    if (fs_flushed) {
      sd.card()->syncDevice();
      //sd.cacheClear();
      while(sd.card()->isBusy()) {}
      fs_flushed = false;
    }
  }
}

static uint16_t readCount;
uint32_t frameSize = 0;
bool frameDeliminatorAcquired = false;
uint32_t liveTimeoutStart = 0;
uint16_t liveTimeoutLimitms = 750;

enum COMMAND_TYPE{
    NONE,
    FRAME_DELIMINATOR,
    TINYTV_TYPE
};

uint8_t commandCheck(uint8_t *jpegBuffer){
  // "0x30 0x30 0x64 0x63" is the start of an avi frame
  if(jpegBuffer[0] == 0x30 && jpegBuffer[1] == 0x30 && jpegBuffer[2] == 0x64 && jpegBuffer[3] == 0x63){
    frameDeliminatorAcquired = true;
    return FRAME_DELIMINATOR;
    
  }else if(jpegBuffer[4] == 'T' && jpegBuffer[5] == 'Y' && jpegBuffer[6] == 'P' && jpegBuffer[7] == 'E'){
    #if !defined(TinyTVKit) && !defined(TinyTVMini)
      cdc.write("TV2");
    #elif !defined(TinyTVKit)
      cdc.write("TVMINI");
    #endif
    return TINYTV_TYPE;
  }else if(jpegBuffer[5] == 'V' && jpegBuffer[6] == 'E' && jpegBuffer[7] == 'R'){
    // Allow for major.minor.patch up to [XXX.XXX.XXX]
    char version[12];

    #if defined(TINYTV_2_COMPILE)
      sprintf(version, "[%u.%u.%u]", TINYTV_2_VERSION_MAJOR, TINYTV_2_VERSION_MINOR, TINYTV_2_VERSION_PATCH);
    #elif defined(TINYTV_MINI_COMPILE)
      sprintf(version, "[%u.%u.%u]", TINYTV_MINI_VERSION_MAJOR, TINYTV_MINI_VERSION_MINOR, TINYTV_MINI_VERSION_PATCH);
    #elif defined(TINYTV_KIT_COMPILE)
      sprintf(version, "[%u.%u.%u]", TINYTV_DIY_VERSION_MAJOR, TINYTV_DIY_VERSION_MINOR, TINYTV_DIY_VERSION_PATCH);
    #endif

    cdc.write(version);
  }

  return NONE;
}


void commandSearch(uint8_t *jpegBuffer){
  while(cdc.available()){
    // Move all bytes from right (highest index) to left (lowest index) in buffer
    jpegBuffer[0] = jpegBuffer[1];
    jpegBuffer[1] = jpegBuffer[2];
    jpegBuffer[2] = jpegBuffer[3];
    jpegBuffer[3] = jpegBuffer[4];
    jpegBuffer[4] = jpegBuffer[5];
    jpegBuffer[5] = jpegBuffer[6];
    jpegBuffer[6] = jpegBuffer[7];
    jpegBuffer[7] = cdc.read();

    if(commandCheck(jpegBuffer) == FRAME_DELIMINATOR){
      break;
    }
  }
}

void JPEGBufferFilled(int);

bool incomingCDCHandler(uint8_t *jpegBuffer, const uint16_t jpegBufferSize, uint16_t &jpegBufferReadCount = readCount){
  if(cdc.available() > 0){
    liveTimeoutStart = millis();

    if(frameDeliminatorAcquired){
      live = true;
      //return fillBuffer(jpegBuffer, jpegBufferSize, jpegBufferReadCount);
      if(frameSize == 0){
        frameSize = (((uint16_t)jpegBuffer[7]) << 24) | (((uint16_t)jpegBuffer[6]) << 16) | (((uint16_t)jpegBuffer[5]) << 8) | ((uint16_t)jpegBuffer[4]);

        if(frameSize >= jpegBufferSize){
          frameSize = 0;
          //jpegBufferReadCount = 0;
          frameDeliminatorAcquired = false;
          cdc.println("ERROR: Received frame size is too big, something went wrong, searching for frame deliminator...");
        }
      }else{
        // If the frame size was determined, get number of bytes to read, check if done filling, then fill if not done
        uint16_t bytesToReadCount = frameSize - (jpegBufferReadCount+1);

        if(bytesToReadCount <= 0){
          frameSize = 0;
          frameDeliminatorAcquired = false;
          JPEGBufferFilled(jpegBufferReadCount);
          jpegBufferReadCount = 0;
          return true;
        }

        // Just in case, check if this read will take us out of bounds, if so, restart (shouldn't happen except for future changes forgetting about this)
        if(jpegBufferReadCount+bytesToReadCount < frameSize){
          jpegBufferReadCount += cdc.read(jpegBuffer + jpegBufferReadCount, bytesToReadCount);
        }else{
          // Not going to get reset by decode so do it here so it doesn't get stuck out of bounds forever
          jpegBufferReadCount = 0;

          frameSize = 0;
          frameDeliminatorAcquired = false;
          cdc.println("ERROR: Tried to place jpeg data out of bounds...");
        }
      }
      if (millis() - liveTimeoutStart >= liveTimeoutLimitms) // Do timeout check if we're waiting for data
      {
        frameSize = 0; 
        frameDeliminatorAcquired = false;
        live = false;
      }
      // Buffer not filled yet, wait for more bytes
      return false;
    }else{
      // Search for deliminator to get back to filling buffers or respond to commands
      commandSearch(jpegBuffer);
    }
  }else if(millis() - liveTimeoutStart >= liveTimeoutLimitms){
    // A timeout is a time to reset states of both jpeg buffers, reset everything
    frameSize = 0; 
    frameDeliminatorAcquired = false;
    live = false;

    // Wait for decoding to finish and then reset incoming jpeg data read counts (otherwise may start at something other than zero next time)
  }

  // No buffer filled, wait for more bytes
  return false;
}

