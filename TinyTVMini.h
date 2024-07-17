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

#include "hardware/pwm.h"

#define TINYTV2_MINI_COMPILE
#define TYPE_STRING "TinyTVMini"
#define TinyTVMini 1

const int VIDEO_X = 0;
const int VIDEO_Y = 0;
const int VIDEO_W = 64;
const int VIDEO_H = 64;
const int VIDEOBUF_SIZE = 1024 * 20;
const int VIDEOBUF_CNT = 2;
const int AUDIOBUF_SIZE = 1024 * 2;

uint8_t sharedBuffer[VIDEOBUF_SIZE * VIDEOBUF_CNT + AUDIOBUF_SIZE];
uint8_t (*videoBuf)[VIDEOBUF_SIZE] = (uint8_t (*)[VIDEOBUF_SIZE])sharedBuffer;
uint8_t (*audioBuf) = (uint8_t (*))(sharedBuffer + VIDEOBUF_SIZE*VIDEOBUF_CNT);

#define DOUBLE_BUFFER true

#define MAX_VIDEO_FILES 100
#define MAX_LFN_LEN 150

TinyScreen display = TinyScreen(RP2040TVMini);
#include "splashes/FileNotFoundSplash_64x64.h"
#include "splashes/NoCardSplash_64x64.h"
#include "splashes/StorageErrorSplash_64x64.h"
#include "splashes/PlaybackErrorSplash_64x64.h"
#include "splashes/MassStorageModeSplash_64x64.h"
#define PLAYBACK_ERROR_SPLASH PlaybackErrorSplash_64x64
#define NO_CARD_ERROR_SPLASH NoCardSplash_64x64
#define STORAGE_ERROR_SPLASH StorageErrorSplash_64x64
#define FILE_NOT_FOUND_SPLASH FileNotFoundSplash_64x64
#define MASS_STORAGE_SPLASH MassStorageModeSplash_64x64


#ifdef DEBUGAPP
extern Adafruit_USBD_CDC cdc;
const void dbgPrint(const char* s) {
  cdc.println(s);
}
const void dbgPrint(const String& s) {
  cdc.println(s);
}
#else
const void dbgPrint(const char* s) {
  (void) s;
}
const void dbgPrint(const String& s) {
  (void) s;
}
#endif

#define IR_INPUT_PIN 1
const uint8_t IR_PIN = 1;
const uint8_t POWER_BTN_PIN = 11;
const uint8_t POWER_OFF_PIN = 20;

const uint8_t AUDIO_PIN = 22;
const uint8_t SPK_EN_PIN = 23;

//Input button pins
const int LEFT_BTN_PIN = 25;
const int RIGHT_BTN_PIN = 0;

volatile bool leftButtonWasPressed = false;
volatile bool rightButtonWasPressed = false;

volatile uint32_t powerButtonWasPressed = 0;

// Pin definitions for SD card
const uint8_t SD_CS = 17;
const uint8_t SD_MISO = 16;
const uint8_t SD_MOSI = 19;
const uint8_t SD_SCK = 18;

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
      spi_read_blocking(spi0 , 0xFF, buf, count);
      return 0;
    }
    // Send a byte.
    void send(uint8_t data) {
      SPI.transfer(data);
    }
    // Send multiple bytes.
    // Replace this function if your board has multiple byte send.
    void send(const uint8_t* buf, size_t count) {
      spi_write_blocking(spi0, buf, count);
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
  SPI.setTX(SD_MOSI);
  SPI.setRX(SD_MISO);
  SPI.setSCK(SD_SCK);
  SPI.setCS(SD_CS);
  SPI.begin(true);  //check argument?

  int error = sd.cardBegin(SD_CONFIG);
  if(!error){
    error = sd.cardBegin(SD_CONFIG);
  }
  return error;
}

int initializeFS() {
  return sd.volumeBegin();
}

void updateButtonStates(inputFlagStruct * inputFlags) {
  if (leftButtonWasPressed && (millis() - leftButtonWasPressed > 20)) {
    if (digitalRead(LEFT_BTN_PIN) == LOW) {
      leftButtonWasPressed = 0;
      inputFlags->volUp = true;
    }
  }
  if (rightButtonWasPressed && (millis() - rightButtonWasPressed > 20)) {
    if (digitalRead(RIGHT_BTN_PIN) == LOW) {
      rightButtonWasPressed = 0;
      inputFlags->channelUp = true;
    }
  }
  if (powerButtonWasPressed && (millis() - powerButtonWasPressed > 20)) {
    if (digitalRead(POWER_BTN_PIN) == LOW) {
      powerButtonWasPressed = 0;
      inputFlags->power = true;
    }
  }
}

void leftButtonPressedInt() {
  leftButtonWasPressed = millis();
}

void rightButtonPressedInt() {
  rightButtonWasPressed = millis();
}

void powerButtonPressedInt() {
  powerButtonWasPressed = millis();
}

void clearPowerButtonPressInt() {
  powerButtonWasPressed = 0;
}

bool powerButtonPressed() {
  return (digitalRead(POWER_BTN_PIN) == LOW);
}

void hardwarePowerOff() {
  digitalWrite(20, HIGH);
}

int audioPinPWMSliceNumber = 0;
int interruptPWMSliceNumber = 0;

void initAudioPin(int pin) {
  digitalWrite(SPK_EN_PIN, LOW); // Speaker disable
  audioPinPWMSliceNumber = pwm_gpio_to_slice_num(pin);
  interruptPWMSliceNumber = audioPinPWMSliceNumber + 1;
  gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&config, 1);// This divider controls the base output PWM frequency
  pwm_config_set_wrap(&config, 255);    // 8 bit audio output
  pwm_init(audioPinPWMSliceNumber, &config, true);
  digitalWrite(SPK_EN_PIN, HIGH); // Speaker enable
  //if (!mute)
}

void initalizePins() {
  pinMode(SPK_EN_PIN, OUTPUT);
  pinMode(POWER_BTN_PIN, OUTPUT);

  pinMode(POWER_BTN_PIN, INPUT_PULLUP);
  pinMode(LEFT_BTN_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BTN_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(POWER_BTN_PIN), powerButtonPressedInt, FALLING);
  attachInterrupt(digitalPinToInterrupt(LEFT_BTN_PIN), leftButtonPressedInt, FALLING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_BTN_PIN), rightButtonPressedInt, FALLING);

  // Speaker enable pin
  pinMode(POWER_OFF_PIN, OUTPUT);
  // Enable the speaker and set the PWM pin up for audio
  initAudioPin(AUDIO_PIN);
}

void pwmInterruptHandler(void);

void setAudioHWSampleRate(int sr) {
  //generate the interrupt at the audio sample rate to set the PWM duty cycle
  pwm_clear_irq(interruptPWMSliceNumber);
  pwm_set_irq_enabled(interruptPWMSliceNumber, true);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, pwmInterruptHandler);
  irq_set_priority(PWM_IRQ_WRAP, 1);
  irq_set_enabled(PWM_IRQ_WRAP, true);
  pwm_config configInt = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&configInt, 1);
  pwm_config_set_wrap(&configInt, (F_CPU / sr) - 1);
  pwm_init(interruptPWMSliceNumber, &configInt, true);
}

#define SET_DAC_LEVEL(x) pwm_set_gpio_level(AUDIO_PIN, uint16_t(x))
#define CLEAR_DAC_IRQ()  pwm_clear_irq(interruptPWMSliceNumber)

extern void setVolume(int);

void volumeUp() {

  if (volumeSetting < 6) {
    volumeSetting++;
  } else {
    volumeSetting = 0; // Wrap audio around on the mini because we have no vol. down
  }
  setVolume(volumeSetting);
}

void volumeDown() {
  if (volumeSetting > 0) {
    volumeSetting--;
  }
  setVolume(volumeSetting);
}
