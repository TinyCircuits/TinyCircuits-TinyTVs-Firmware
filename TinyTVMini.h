
#define TINYTV2_MINI_COMPILE
#define TinyTVMini 1

const int VIDEO_X = 0;
const int VIDEO_Y = 0;
const int VIDEO_W = 64;
const int VIDEO_H = 64;
const int VIDEOBUF_SIZE = 1024 * 20;
const int VIDEOBUF_CNT = 2;

TinyScreen display = TinyScreen(RP2040TVMini);
#include "splashes/FileNotFoundSplash_64x64.h"
#include "splashes/NoCardSplash_64x64.h"
#include "splashes/StorageErrorSplash_64x64.h"
#include "splashes/PlaybackErrorSplash_64x64.h"

#ifdef DEBUGAPP
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


const uint8_t IR_PIN = 1;

const uint8_t BTN_1 = 11;

const int LEFT_BTN_PIN = 25;
const int RIGHT_BTN_PIN = 0;

const uint8_t AUDIO_PIN = 22;
const uint8_t SPK_EN = 23;

volatile bool leftButtonWasPressed = false;
volatile bool rightButtonWasPressed = false;

// Pin definitions for SD card
const uint8_t SD_CS = 17;
const uint8_t SD_MISO = 16;
const uint8_t SD_MOSI = 19;
const uint8_t SD_SCK = 18;
const SdSpiConfig SD_CONFIG(SD_CS, DEDICATED_SPI, min(F_CPU / 4, 50000000), &SPI);

void initalizePins() {
  pinMode(BTN_1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_1), powerButtonPressedInt, FALLING);

  // OLED boost supply pin
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  pinMode(LEFT_BTN_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(LEFT_BTN_PIN), leftButtonPressedInt, FALLING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_BTN_PIN), rightButtonPressedInt, FALLING);

  // Speaker enable pin
  pinMode(23, OUTPUT);
  // Enable the speaker and set the PWM pin up for audio
   initAudioPin(AUDIO_PIN);
}

int initializeSDcard() {
  // Set SPI up and make sure the SD card is working
  SPI.setTX(SD_MOSI);
  SPI.setRX(SD_MISO);
  SPI.setSCK(SD_SCK);
  SPI.setCS(SD_CS);
  SPI.begin(true);  //check argument?

  return sd.cardBegin(SD_CONFIG);
}


int initializeFS() {
  return sd.volumeBegin();
}

void updateButtonStates()
{
  // Encoder/button input
  if (leftButtonWasPressed) {
    leftButtonWasPressed = false;
    volUpInput = true;
  }
  if (rightButtonWasPressed) {
    rightButtonWasPressed = false;
    channelUpInput = true;
  }
  if (powerButtonWasPressed) {
    powerButtonWasPressed = false;
    powerInput = true;
  }
}

void leftButtonPressedInt() {
  leftButtonWasPressed = true;
}

void rightButtonPressedInt() {
  rightButtonWasPressed = true;
}

void powerButtonPressedInt() {
  powerButtonWasPressed = true;
}

void clearPowerButtonPressInt() {
  powerButtonWasPressed = false;
}
bool powerButtonPressed() {
  return (digitalRead(BTN_1) == LOW);
}
