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
  if (secondCoreSD /*&& !ejected*/ ) {
    while (lbaToWriteCount || lbaToReadCount);
    volatile int count = bufsize / 512;
    lbaToRead = lba * sectorLBACount;
    lbaToReadPos = (uint8_t*)buffer;
    lbaToReadCount = count;
    while (lbaToReadCount == count) {};
    return bufsize;
  } else if (/*!ejected*/ 1) {
    return sd.card()->readSectors(lba * sectorLBACount, (uint8_t *)buffer, bufsize / 512) ? bufsize : -1;
  }
  return 0;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
  if (secondCoreSD /* && !ejected*/ ) {
    while (lbaToWriteCount || lbaToReadCount);
    memcpy(lbaWriteBuff, buffer, bufsize);
    int count = bufsize / 512;
    lbaToWrite = lba * sectorLBACount;
    lbaToWritePos = 0;
    lbaToWriteCount = count;
    return bufsize;
  } else if (/*!ejected*/ 1) {
    return sd.card()->writeSectors(lba * sectorLBACount, buffer, bufsize / 512) ? bufsize : -1;
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
  //(void) power_condition;
  //  cdc.print(power_condition);
  //  cdc.print(" ");
  //  cdc.print(load_eject);
  //  cdc.print(" ");
  //  cdc.println(start);

  //ejected = true;

  if (load_eject)
  {
    if (start)
    {
      mscStart = true;
    }
    else
    {
      ejected = true;
      //return false;
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
    //tud_ready doesn't seem to work, rely on ejected- doesn't seem to work in macos?
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
