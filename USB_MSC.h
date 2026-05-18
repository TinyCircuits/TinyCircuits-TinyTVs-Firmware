//-------------------------------------------------------------------------------
//  TinyCircuits TinyTV Firmware

#ifdef ARDUINO_ARCH_RP2040
#include "Adafruit_TinyUSB_API.h"
#include "tusb.h"
// Arduino yield() calls TinyUSB_Device_Task() then TinyUSB_Device_FlushCDC().
// CDC TX flush during MSC starves composite traffic on macOS; run USB stack only.
static inline void msc_yield_usb_only(void) {
  TinyUSB_Device_Task();
  TinyUSB_Device_Task();
}

// main.cpp calls TinyUSB_Device_Init() before setup(). The host can finish enumerating
// that first configuration (CDC from TinyUSBDevice.begin) while setup() still runs.
// setup() then clears descriptors and adds CDC + MSC — but without a detach/attach the
// host keeps the old view (Adafruit TinyUSB issue #96). macOS then shows MSC in Disk
// Utility but fails to mount the FAT volume. Drop D+ pull-up before rebuilding interfaces,
// and re-enable only after READ CAPACITY data (block_count) is valid.
static inline void USBMSC_bus_detach_before_rebuild(void) {
  if (tud_inited()) {
    tud_disconnect();
    delay(20);
  }
}

static inline void USBMSC_bus_attach_after_msc_geometry_ready(void) {
  if (tud_inited()) {
    tud_connect();
    delay(10);
  }
}
#endif
//
//  Changelog:
//  03/25/2026 MP4 playback update
//  05/26/2023 Initial Release for TinyTV 2/Mini
//  02/08/2023 Cross-platform base committed
//
//  Written by Mason Watmough, Ben Rose, and Jason Marcum for TinyCircuits, http://TinyCircuits.com
//
//-------------------------------------------------------------------------------


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


uint32_t lastMSCRead = 0;
uint32_t lastMSCWrite = 0;

bool lastState = false;
bool ended = false;

int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize)
{
  if (bufsize == 0 || (bufsize % 512) != 0) {
    return -1;
  }
  if (secondCoreSD /*&& !ejected*/ ) {
    while (lbaToWriteCount || lbaToReadCount);
    volatile int count = bufsize / 512;
    lbaToRead = lba * sectorLBACount;
    lbaToReadPos = (uint8_t*)buffer;
    lbaToReadCount = count;
    while (lbaToReadCount == count) {};
    return bufsize;
  } else if (/*!ejected*/ 1) {
    lastMSCRead = millis();
    return sd.card()->readSectors(lba * sectorLBACount, (uint8_t *)buffer, bufsize / 512) ? bufsize : -1;
  }
  return 0;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
  if (bufsize == 0 || (bufsize % 512) != 0) {
    return -1;
  }
  if (secondCoreSD /* && !ejected*/ ) {
    while (lbaToWriteCount || lbaToReadCount);
    memcpy(lbaWriteBuff, buffer, bufsize);
    int count = bufsize / 512;
    lbaToWrite = lba * sectorLBACount;
    lbaToWritePos = 0;
    lbaToWriteCount = count;
    return bufsize;
  } else if (/*!ejected*/ 1) {
    lastMSCWrite = millis();
    bool writeSuccess = sd.card()->writeSectors(lba * sectorLBACount, buffer, bufsize / 512);
    int errorCount = 0;
    while (writeSuccess == false && errorCount < 3) {
      if (!sd.cardBegin(SD_CONFIG)) {
        // SD card not responding, break out of loop and report error
      }
      writeSuccess = sd.card()->writeSectors(lba * sectorLBACount, buffer, bufsize / 512);
      errorCount++;
    }
    return writeSuccess ? bufsize : -1;
  }
  return 0;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb(void)
{
  if (secondCoreSD /*&& !ejected*/ ) {
    fs_flushed = true;
  } else if (/*!ejected*/ 1) {
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
  // Embedded microSD: acknowledge START STOP UNIT but do not set ejected on load_eject+!start.
  // macOS sends that sequence during mount / diskarbitration; the old code set ejected=true,
  // handleUSBMSC() then removed MSC callbacks while the LUN stayed enumerated — Disk Utility
  // showed TINYTV as Not Mounted with 0 B free until replug.
  if (load_eject && start) {
    mscStart = true;
  }
  return true;
}
#ifdef __cplusplus
}
#endif

void USBMSCInit() {
  usb_msc.setID("TinyCircuits", "RP2040TV", "1.0");
  //usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
  usb_msc.setUnitReady(false);
  usb_msc.begin();
}
void USBMSCReady() {
  uint32_t block_count = sd.card()->sectorCount();
  usb_msc.setCapacity(block_count, 512);
}

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

bool USBMSCRecentActivity() {
  if (millis() - lastMSCRead < 500)
    return true;
  if (millis() - lastMSCWrite < 500)
    return true;
  return false;
}

bool handleUSBMSC(bool stopMSC) {
  if (mscActive) {
    // Remain in MSC until the user stops (power) or USB disconnects. Do not use ejected from
    // START STOP UNIT — macOS uses that during probe (see tud_msc_start_stop_cb).
    // Avoid tud_ready(): on macOS it often stays false during normal MSC use.
    //
    // Stay active while SET_CONFIGURATION is pending: tud_mounted() may be false briefly even
    // though the cable is attached (tud_connected). After bus resets, tud_connected() can also
    // glitch false for a few hundred ms — debounce before treating that as unplug so we do not
    // tear down MSC and strand the user (power would still set stopMSC).
#ifdef ARDUINO_ARCH_RP2040
    static uint32_t mscDisconnectDebounceStart = 0;
    const uint32_t kMscDisconnectHoldMs = 500;
#endif

    if (!stopMSC) {
      if (tud_mounted()) {
#ifdef ARDUINO_ARCH_RP2040
        mscDisconnectDebounceStart = 0;
        msc_yield_usb_only();
#else
        yield();
#endif
        return true;
      }
      if (tud_connected()) {
#ifdef ARDUINO_ARCH_RP2040
        mscDisconnectDebounceStart = 0;
        msc_yield_usb_only();
#else
        yield();
#endif
        return true;
      }
#ifdef ARDUINO_ARCH_RP2040
      // Unplug: no link — but wait out brief post-reset glitches.
      uint32_t now = millis();
      if (mscDisconnectDebounceStart == 0) {
        mscDisconnectDebounceStart = now;
      }
      if ((uint32_t)(now - mscDisconnectDebounceStart) < kMscDisconnectHoldMs) {
        msc_yield_usb_only();
        return true;
      }
      mscDisconnectDebounceStart = 0;
#endif
    } else {
#ifdef ARDUINO_ARCH_RP2040
      mscDisconnectDebounceStart = 0;
#endif
    }

    for (int i = 0; i < 50 || USBMSCRecentActivity(); i++) {
      delay(1);
#ifdef ARDUINO_ARCH_RP2040
      msc_yield_usb_only();
#else
      yield();
#endif
    }
    usb_msc.setUnitReady(false);
    usb_msc.setReadWriteCallback(nullptr, nullptr, nullptr);
    for (int i = 0; i < 50 || USBMSCRecentActivity(); i++) {
      delay(1);
#ifdef ARDUINO_ARCH_RP2040
      msc_yield_usb_only();
#else
      yield();
#endif
    }
    mscActive = false;
    sd.card()->syncDevice();
    sd.cacheClear();
    while (sd.card()->isBusy()) {}
    //usb_msc.setReadWriteCallback(nullptr, nullptr, nullptr);
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
      while (sd.card()->isBusy()) {}
      fs_flushed = false;
    }
  }
}
