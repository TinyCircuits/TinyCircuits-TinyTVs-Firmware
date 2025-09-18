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

#define TINYTV_KIT_COMPILE
#define TYPE_STRING "TinyTVKit"
#define TinyTVKit 1

const int VIDEO_X = 0;
const int VIDEO_Y = 0;
const int VIDEO_W = 96;
const int VIDEO_H = 64;
const int VIDEOBUF_SIZE = 1024 * 3;
const int VIDEOBUF_CNT = 1;
const int AUDIOBUF_SIZE = 1024 * 2;

uint8_t sharedBuffer[VIDEOBUF_SIZE*VIDEOBUF_CNT + AUDIOBUF_SIZE];
uint8_t (*videoBuf)[VIDEOBUF_SIZE] = (uint8_t (*)[VIDEOBUF_SIZE])sharedBuffer;
uint8_t (*audioBuf) = (uint8_t (*))(sharedBuffer + VIDEOBUF_SIZE*VIDEOBUF_CNT);

#define DOUBLE_BUFFER false

#define MAX_VIDEO_FILES 25
#define MAX_LFN_LEN 50

TinyScreen display = TinyScreen(TinyScreenPlus);
#include "splashes/FileNotFoundSplash_96x64.h"
#include "splashes/NoCardSplash_96x64.h"
#include "splashes/StorageErrorSplash_96x64.h"
#include "splashes/PlaybackErrorSplash_96x64.h"
#define PLAYBACK_ERROR_SPLASH PlaybackErrorSplash_96x64
#define NO_CARD_ERROR_SPLASH NoCardSplash_96x64
#define STORAGE_ERROR_SPLASH StorageErrorSplash_96x64
#define FILE_NOT_FOUND_SPLASH FileNotFoundSplash_96x64



#ifdef DEBUGAPP
const void dbgPrint(const char* s) {
  SerialUSB.println(s);
}
const void dbgPrint(const String& s) {
  SerialUSB.println(s);
}
#else
const void dbgPrint(const char* s) {
  (void) s;
}
const void dbgPrint(const String& s) {
  (void) s;
}
#endif


#define IR_INPUT_PIN 3
//const uint8_t IR_PIN = 5;
const uint8_t AUDIO_PIN = A0;
const uint8_t SD_CS = 10;

const uint8_t leftUpButtonPin = TSP_PIN_BT4;
const uint8_t rightUpButtonPin = TSP_PIN_BT1;
const uint8_t leftDownButtonPin = TSP_PIN_BT3;
const uint8_t rightDownButtonPin = TSP_PIN_BT2;

volatile uint32_t leftUpButtonWasPressed = 0;
volatile uint32_t rightUpButtonWasPressed = 0;
volatile uint32_t leftDownButtonWasPressed = 0;
volatile uint32_t rightDownButtonWasPressed = 0;


#if SPI_DRIVER_SELECT == 3  // Must be set in SdFat/src/SdFatConfig.h

// This is a simple driver based on the the standard SPI.h library.
// You can write a driver entirely independent of SPI.h.
// It can be optimized for your board or a different SPI port can be used.
// The driver must be derived from SdSpiBaseClass.
// See: SdFat/src/SpiDriver/SdSpiBaseClass.h
class MySpiClass : public SdSpiBaseClass {
  public:
    // Activate SPI hardware with correct speed and mode.
    void activate() {
      SPI.beginTransaction(m_spiSettings);
    }
    // Initialize the SPI bus.
    void begin(SdSpiConfig config) {
      //if (config.spiPort) {
      //  m_spi = config.spiPort;
      //} else {
      //  m_spi = &SPI;
      //}
      (void)config;
      SPI.begin();
    }
    // Deactivate SPI hardware.
    void deactivate() {
      SPI.endTransaction();
    }
    // Receive a byte.
    uint8_t receive() {
      return SPI.transfer(0XFF);
    }
    // Receive multiple bytes.
    // Replace this function if your board has multiple byte receive.
    uint8_t receive(uint8_t* buf, size_t count) {
      for (size_t i = 0; i < count; i++) {
        SERCOM1->SPI.DATA.bit.DATA = 0XFF;
        while (SERCOM1->SPI.INTFLAG.bit.DRE == 0 || SERCOM1->SPI.INTFLAG.bit.RXC == 0);
        buf[i] = SERCOM1->SPI.DATA.bit.DATA;
      }
      return 0;
    }
    // Send a byte.
    void send(uint8_t data) {
      SPI.transfer(data);
    }
    // Send multiple bytes.
    // Replace this function if your board has multiple byte send.
    void send(const uint8_t* buf, size_t count) {
      uint8_t temp;
      SERCOM1->SPI.DATA.bit.DATA = buf[0];
      for (size_t j = 1; j < count; j++) {
        temp = buf[j];
        while (SERCOM1->SPI.INTFLAG.bit.DRE == 0 || SERCOM1->SPI.INTFLAG.bit.RXC == 0);
        SERCOM1->SPI.DATA.bit.DATA = temp;
      }
      while (SERCOM1->SPI.INTFLAG.bit.DRE == 0 || SERCOM1->SPI.INTFLAG.bit.RXC == 0);
    }
    void setSckSpeed(uint32_t maxSck) {
      //(void)maxSck;
      m_spiSettings = SPISettings(maxSck, MSBFIRST, SPI_MODE0);
    }

  private:
    SPISettings m_spiSettings;
    //SPIClass* m_spi;
} customSPI;
#else  // SPI_DRIVER_SELECT
#error SPI_DRIVER_SELECT must be set to 3 in SdFat/src/SdFatConfig.h
#endif  // SPI_DRIVER_SELECT

const SdSpiConfig SD_CONFIG(SD_CS, DEDICATED_SPI, min(F_CPU / 4, 50000000), &customSPI);


int initializeSDcard() {
  // Set SPI up and make sure the SD card is working
  SPI.begin();
  
  int error = sd.cardBegin(SD_CONFIG);
  if(!error){
    error = sd.cardBegin(SD_CONFIG);
  }
  return error;
}


int initializeFS() {
  return sd.volumeBegin();
}

void updateButtonStates(inputFlagStruct * inputFlags){
  if (leftUpButtonWasPressed && (millis() - leftUpButtonWasPressed > 30)) {
    leftUpButtonWasPressed = false;
    inputFlags->channelUp = true;
  }
  if (rightUpButtonWasPressed && (millis() - rightUpButtonWasPressed > 30)) {
    rightUpButtonWasPressed = false;
    inputFlags->volUp = true;
  }
  if (leftDownButtonWasPressed && (millis() - leftDownButtonWasPressed > 30)) {
    leftDownButtonWasPressed = false;
    inputFlags->channelDown = true;
  }
  if (rightDownButtonWasPressed && (millis() - rightDownButtonWasPressed > 30)) {
    rightDownButtonWasPressed = false;
    inputFlags->volDown = true;
  }
}



void leftUpButtonPressedInt() {
  leftUpButtonWasPressed = millis();
}
void rightUpButtonPressedInt() {
  rightUpButtonWasPressed = millis();
}
void leftDownButtonPressedInt() {
  leftDownButtonWasPressed = millis();
}
void rightDownButtonPressedInt() {
  rightDownButtonWasPressed = millis();
}



void initalizePins() {
  pinMode(leftUpButtonPin, INPUT_PULLUP);
  pinMode(rightUpButtonPin, INPUT_PULLUP);
  pinMode(leftDownButtonPin, INPUT_PULLUP);
  pinMode(rightDownButtonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(leftUpButtonPin), leftUpButtonPressedInt, FALLING);
  attachInterrupt(digitalPinToInterrupt(rightUpButtonPin), rightUpButtonPressedInt, FALLING);
  attachInterrupt(digitalPinToInterrupt(leftDownButtonPin), leftDownButtonPressedInt, FALLING);
  attachInterrupt(digitalPinToInterrupt(rightDownButtonPin), rightDownButtonPressedInt, FALLING);

  analogWrite(AUDIO_PIN, analogRead(AUDIO_PIN));
}


bool tcIsSyncing() {
  return TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY;
}

void setAudioHWSampleRate(int sr) {
  // Enable GCLK for TCC2 and TC5 (timer counter input clock)
  GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
  while (GCLK->STATUS.bit.SYNCBUSY);
  // Reset TCx
  TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (tcIsSyncing());
  while (TC5->COUNT16.CTRLA.bit.SWRST);
  // Set Timer counter Mode to 16 bits
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
  // Set TC5 mode as match frequency
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1;
  TC5->COUNT16.CC[0].reg = (uint16_t) (SystemCoreClock / sr - 1);
  while (tcIsSyncing());
  // Configure interrupt request
  NVIC_DisableIRQ(TC5_IRQn);
  NVIC_ClearPendingIRQ(TC5_IRQn);
  NVIC_SetPriority(TC5_IRQn, 0);
  NVIC_EnableIRQ(TC5_IRQn);
  // Enable the TC5 interrupt request
  TC5->COUNT16.INTENSET.bit.MC0 = 1;
  while (tcIsSyncing());
  // Enable TC
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
  while (tcIsSyncing());
}


extern void setVolume(int);

void volumeUp() {
  if (volumeSetting < 6) {
    volumeSetting++;
  }
  setVolume(volumeSetting);
}

void volumeDown() {
  if (volumeSetting > 0) {
    volumeSetting--;
  }
  setVolume(volumeSetting);
}
