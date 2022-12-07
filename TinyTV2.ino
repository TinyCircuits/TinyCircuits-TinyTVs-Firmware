//-------------------------------------------------------------------------------
//  TinyCircuits RP2040TV Video Player
//
//  Board: Raspberry Pi Pico
//  CPU Speed: 200MHz TinyTV 2/ 50MHZ TinyTV Mini
//  Optimization: Even more (-O3)
//  USB Stack: Adafruit TinyUSB
//
//  Changelog:
//  08/12/2022 Handed off the keys to the kingdom
//
//  Written by Mason Watmough for TinyCircuits, http://TinyCircuits.com
//
// packages\rp2040\hardware\rp2040\2.5.2\variants\rpipico  ->  #define PIN_LED        (12u)
// packages\rp2040\hardware\rp2040\2.5.2\libraries\Adafruit_TinyUSB_Arduino\src\arduino\ports\rp2040 tweaked CFG_TUD_MSC_EP_BUFSIZE to 512*8
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

//#define TinyTVMini 0

// Uncomment to compile debug version
//#define DEBUGAPP (true)

//TinyIRReceiver library wants this..
#define IR_INPUT_PIN 1

// This include order matters
#include <SPI.h>
#include <TFT_eSPI.h>               // Custom for OLED
#include <SdFat.h>                  // Custom SdFat for RP2040
#include "TinyIRReceiver.hpp"       // Unmodified IR library
#include <JPEGDEC.h>                // minor customization
#include "hardware/pwm.h"           // RP2040 API
#include "src/uraster/uraster.hpp"  // Non-video graphics library
#include "globals_and_buffers.h"    // Big fat statically-allocate-everything header
#include "USB_MSC.h"                // Adafruit TinyUSB callbacks, and kinda hacky hathach tinyUSB start_stop_cb implementation


void setup() {
  initalizePins();
  // Do MSC setup quickly so PC recognizes us as a mass storage device
  USBMSCInit();
  initializeDisplay();
//#ifdef DEBUGAPP
  Serial.begin(115200);
//#endif
  // Wait for the serial monitor to wake up in debug mode
#ifdef DEBUGAPP
  while (!Serial) {}
#endif
  if (!initializeSDcard()) {
    displayCardNotFound();
    while (1) {}
  }

  //USBMSCLoop();
  clearPowerButtonPressInt();

//#ifdef DEBUGAPP
//  Serial.begin(115200);
//#endif

  loadSettings();
  loadVideoList();

  setAudioSampleRate(10000);
  targetFrameTime = (1000000) / 24;

  // Open a video
  if (doStaticEffects) effects.startChangeChannelEffect();
  nextVideo();

  if (!initPCIInterruptForTinyReceiver()) {
    Serial.println("No interrupt available for IR_INPUT_PIN");
  }
}


void loop() {
  if (justConnected()) {
    setAudioSampleRate(100);
    delay(10);
    yield();
    delay(10);
    yield();
    delay(10);
    yield();
    delay(10);
    yield();
    delay(10);
    yield();
    USBMSCLoop();
    clearPowerButtonPressInt();

    loadSettings();
    loadVideoList();
    nextVideo();
  }

  // IR remote using NEC codes for next/previous channel, volume up and down, and mute
  IRInput();
  // Hardware encoder/button input
  updateButtonStates();

  // Handle any input flags
  if (powerInput) {
    powerInput = false;
    if (!TVscreenOffMode) {
      TVscreenOffMode = true;
      TVscreenOffModeStartTime = millis();
      if (doStaticEffects) effects.startChangeChannelEffect();
      playWhiteNoise = false;
      pauseRadius = 120;
      sampleIndex = 0;
      loadedSampleIndex = sampleIndex;
      //sampleIndex = 0;
      //loadedSampleIndex = AUDIOBUF_SIZE - 1;
      //memset(audioBuf, 127, AUDIOBUF_SIZE);
      //memcpy(audioBuf, shutoff, AUDIOBUF_SIZE);
      //sleep_ms(100);//wait to play?
      while (pauseRadius > 3) tubeOffEffect();
      //      digitalWrite(9, HIGH);
      //?digitalWrite(SPK_EN, LOW);
      clearDisplay();
    } else {
      TVscreenOffMode = false;
      if (doStaticEffects) effects.startChangeChannelEffect();
      playWhiteNoise = false;
      sampleIndex = 0;
      loadedSampleIndex = sampleIndex;
      showChannelTimer = 120;
      paused = false;

      //tsMillisInitial += millis() - lowPowerStartTime;
      ////    digitalWrite(9, LOW);
      //if (!mute) digitalWrite(SPK_EN, HIGH);
    }
  }
  if (muteInput) {
    muteInput = false;
    if (!TVscreenOffMode) {
      setMute(!isMute());
    }
  }
  if (channelUpInput) {
    channelUpInput = false;
    if (!TVscreenOffMode) {
      if (autoplayMode == 2) seekLivePos = true;
      if (doStaticEffects) effects.startChangeChannelEffect();
      nextVideo();
    }
  }
  if (channelDownInput) {
    channelDownInput = false;
    if (!TVscreenOffMode) {
      if (autoplayMode == 2) seekLivePos = true;
      if (doStaticEffects) effects.startChangeChannelEffect();
      prevVideo();
    }
  }
  if (volUpInput) {
    volUpInput = false;
    if (!TVscreenOffMode) {
      soundVolume += 32;
      if (soundVolume >= 256) soundVolume = 256;
      showVolumeTimer = 120;
    }
  }
  if (volDownInput) {
    volDownInput = false;
    if (!TVscreenOffMode) {
      soundVolume -= 32;
      if (soundVolume < 0) soundVolume = 0;
      showVolumeTimer = 120;
    }
  }

  if (TVscreenOffMode) {
    // Turn TV off after 2 minutes in screen off mode
    if (millis() - TVscreenOffModeStartTime > 1000 * 60 * 2) {
      digitalWrite(20, HIGH);
    }
    return;
  }


  uint64_t t0 = time_us_64();

  if (settingsNeedSaved)
  {
    dbgPrint("Saving settings file");
    saveSettings();
    settingsNeedSaved = false;
    dbgPrint("Saved settings file");
  }






  uint8_t chunkHeader[12];
  if (infile.read(chunkHeader, 8) != 8)
  {
    dbgPrint("AVI end / EOF reached or read error");
    if (doStaticEffects) effects.startChangeChannelEffect();
    // Find a new video to play or loop
    if (autoplayMode != 0) {
      nextVideo();
    } else {
      startVideo("");
    }
    return;
  }
  uint32_t len = getInt(chunkHeader + 4);


  if (strncmp((char *)chunkHeader + 0, "01wb", 4) == 0 && len > 0) {
    // Found a chunk of audio, load it into the buffer
    if (len < AUDIOBUF_SIZE) {
      int readLen = 0;
      while (len - readLen > 0) {
        uint32_t r = infile.read((uint8_t*)audioBuf + (loadedSampleIndex), min((AUDIOBUF_SIZE - (loadedSampleIndex)), len - readLen));
        readLen += r;
        loadedSampleIndex += r;
        if ((loadedSampleIndex) >= AUDIOBUF_SIZE)
        {
          loadedSampleIndex -= AUDIOBUF_SIZE;
        }
      }
    }
  } else if (strncmp((char *)chunkHeader + 0, "00dc", 4) == 0 && len > 0) {
    // Found a chunk of video, decode it

    // Read the compressed JFIF data into the video buffer
    infile.read(videoBuf[currentWriteBuf], min(len, VIDEOBUF_SIZE));
    decoderDataLength = len;

    // If core 2 is still decoding, wait
    while (decodingFrame || paused) {
      delay(1);
      yield();
    }

    // Swap to the offhand buffer for the next read while core2 is decoding
    currentWriteBuf = 1 - currentWriteBuf;
    frameReady = true;

    while ((int64_t(time_us_64() - framerateHelper) < (targetFrameTime - 3000))) {
      delay(1);
      yield();
    }

    while (audioSamplesInBuffer() > 1000) {
      delay(1);
      yield();
    }

    // Update playback timing stuff
    framerateHelper = time_us_64();
  }

  if ((len & 1) != 0) { //padding
    infile.seekCur(1);
  }

  uint64_t t1 = time_us_64();
  //dbgPrint("took " + String(uint32_t(t1 - t0)) + "us");

}

void setup1()
{

}

void loop1()
{
  core2Loop();
}
