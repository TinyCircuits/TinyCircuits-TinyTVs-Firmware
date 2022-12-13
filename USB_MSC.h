//-------------------------------------------------------------------------------
//  TinyCircuits RP2040TV Video Player, USB Callbacks Component
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


#include <Adafruit_TinyUSB.h>
Adafruit_USBD_MSC usb_msc;
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

//void _sdWait()
//{
//  while (sd.card()->isBusy()) {
//    delay(1);
//  }
//  return;
//}

int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize)
{
  Serial.print("Reading "); Serial.print(bufsize); Serial.print(" from "); Serial.println(lba);
  //  if (!mscActive) {
  //    Serial.print("skipping\n");
  //    return 0;
  //  }
  if (secondCoreSD) {
    while (lbaToWriteCount || lbaToReadCount);
    volatile int count = bufsize / 512;
    lbaToRead = lba * sectorLBACount;
    lbaToReadPos = (uint8_t*)buffer;
    lbaToReadCount = count;
    while (lbaToReadCount == count) {};
    return bufsize;
  } else {
    return sd.card()->readSectors(lba * sectorLBACount, (uint8_t *)buffer, bufsize / 512) ? bufsize : -1;
  }
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
  Serial.print("Writing "); Serial.print(bufsize); Serial.print(" to "); Serial.println(lba);
  //  if (!mscActive) {
  //    Serial.print("skipping\n");
  //    return 0;
  //  }
  if (secondCoreSD) {
    while (lbaToWriteCount || lbaToReadCount);
    memcpy(lbaWriteBuff, buffer, bufsize);
    int count = bufsize / 512;
    lbaToWrite = lba * sectorLBACount;
    lbaToWritePos = 0;
    lbaToWriteCount = count;
    return bufsize;
  } else {
    return sd.card()->writeSectors(lba * sectorLBACount, buffer, bufsize / 512) ? bufsize : -1;
  }
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb(void)
{
  Serial.println("flush CB");
  if (secondCoreSD) {
    fs_flushed = true;
  } else {
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
/*
  void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
  {
  (void) lun;

  block_count = sd.card()->sectorCount();
  block_size  = 512;
  }
*/
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
  //usb_msc.setUnitReady(true);
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
    if (count < 100 && !ejected && !stopMSC) {
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
    //ejected = false;
    dbgPrint("Media ejected.");
    sd.card()->syncDevice();
    sd.cacheClear();
    ended = true;
  }
  return false;
}


void MSCloopCore1() {
  if (secondCoreSD) {
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
      sd.cacheClear();
      fs_flushed = false;
    }
  }
}
