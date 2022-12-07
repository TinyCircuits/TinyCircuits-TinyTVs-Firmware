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
FatVolume* msc_vol;
bool ejected = false;

bool fs_flushed = false;
uint64_t last_write = 0;

uint32_t lbaToRead = 0;
uint8_t* lbaToReadPos = NULL;
uint32_t lbaToReadCount = 0;

uint32_t lbaToWrite = 0;
uint8_t lbaToWritePos = 0;
uint32_t lbaToWriteCount = 0;

uint8_t lbaWriteBuff[512 * 8];

void _sdWait()
{
  while (sd.card()->isBusy()) {
    delay(1);
  }
  return;
}

int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize)
{
  //Serial.println("Reading from " + String(lba));
  const int32_t ret = sd.card()->readSectors(lba, (uint8_t*) buffer, (bufsize >> 9)) ? bufsize : -1;
  return ret;
  //  int count = bufsize / 512;
  //  const int32_t ret = sd.card()->readSectors(lba, (uint8_t*) buffer, 1) ? bufsize : -1;
  //  lbaToRead = lba + 1;
  //  lbaToReadPos = (uint8_t*)buffer + 512;
  //  lbaToReadCount = count - 1;
  //  return bufsize;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
  const int32_t ret = (sd.card()->writeSectors(lba, buffer, (bufsize >> 9)) == true) ? bufsize : -1;
  fs_flushed = false;
  return ret;
  /*
    memcpy(lbaWriteBuff,buffer,bufsize);
    int count = bufsize / 512;
    lbaToWrite = lba;
    lbaToWritePos = 0;
    lbaToWriteCount = count;
    return bufsize;
  */
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb(void)
{
  fs_flushed = true;
}

// This callback is a C symbol because our Adafruit USB version doesn't expose a function pointer to it

#ifdef __cplusplus
extern "C" {
#endif
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
  (void) lun;
  (void) power_condition;
  if (load_eject)
  {
    if (start)
    {
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

bool lastState = false;
bool justConnected() {
  if (tud_connected()) {
    if (lastState == false) {
      lastState = true;
      return true;
    }
  } else {
    lastState = false;
  }
  return false;
}

extern void displayUSBMSCmessage();
extern bool powerButtonPressed();
void USBMSCLoop() {
  //Set up mass storage and stay in this mode while the user is moving files around
  if (tud_connected()) {
    msc_vol = sd.vol();
    uint32_t block_count = sd.card()->sectorCount();
    usb_msc.setCapacity(block_count, 512);
    usb_msc.setUnitReady(true);
    displayUSBMSCmessage();
    while (tud_ready() && !ejected && !powerButtonPressed()) {
      yield();
      if (lbaToReadCount) {
        sd.card()->readSectors(lbaToRead, (uint8_t*) lbaToReadPos, 1);
        lbaToRead += 1;
        lbaToReadCount -= 1;
        lbaToReadPos += 512;
      }/*
          if (lbaToWriteCount) {
            sd.card()->writeSectors(lbaToWrite, (uint8_t*)lbaWriteBuff + lbaToWritePos, 1);
            lbaToWrite += 1;
            lbaToWriteCount -= 1;
            lbaToWritePos += 512;
          }*/
      yield();
    }
    usb_msc.setUnitReady(false);
    //delay(100);
    //TinyUSBDevice.detach();
    //delay(100);
    //TinyUSBDevice.attach();
    //dbgPrint("Media ejected.");
  }
}
