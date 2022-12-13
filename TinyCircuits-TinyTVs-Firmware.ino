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

//#define TinyTVMini 1

// Uncomment to compile debug version
// #define DEBUGAPP (true)

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
#include <Adafruit_TinyUSB.h>
#include "screenEffects.h"
#include "JPEGStreamer.h"
#include "globals_and_buffers.h"    // Big fat statically-allocate-everything header
#include "USB_MSC.h"                // Adafruit TinyUSB callbacks, and kinda hacky hathach tinyUSB start_stop_cb implementation


int nextVideoError = 0;
int prevVideoError = 0;
bool showNoVideoError = false;
uint64_t TVscreenOffModeStartTime = 0;

void setup() {
  initalizePins();
  
  Serial.end();
  cdc.begin(0);

  // Do MSC setup quickly so PC recognizes us as a mass storage device
  USBMSCInit();
  initializeDisplay();
  //Serial.begin(115200);
  // Wait for the serial monitor to wake up in debug mode

  // Set crop radius to TinyTV 2's best looking value;
  #ifdef TinyTVMini
    effects.setCropRadius(8);
  #else
    effects.setCropRadius(25);
  #endif

  if (!initializeSDcard()) {
    displayCardNotFound();
    while (1) {
    }
  }
  USBMSCReady();

  if (!initializeFS()) {
    displayFileSystemError();
    while (1) {
      USBMSCStart();
      handleUSBMSC(false);
    }
  }

  loadSettings();
  if (loadVideoList()) {
    if (nextVideo()) {
      nextVideoError = millis();
    }
  } else {
    showNoVideoError = true;
  }


  setAudioSampleRate(10000);
  targetFrameTime = (1000000) / 24;


  if (!initPCIInterruptForTinyReceiver()) {
    cdc.println("No interrupt available for IR_INPUT_PIN");
  }

}


void loop() {
  if (USBJustConnected()) {
    setAudioSampleRate(100);
    displayUSBMSCmessage();
    USBMSCStart();
  }
  if (handleUSBMSC(powerButtonPressed())) {
    //USBMSC active:
    return;
  }
  if (USBMSCJustStopped()) {
    //USBMSC ejected, return to video playback:
    clearPowerButtonPressInt();
    loadSettings();
    if (loadVideoList()) {
      showNoVideoError = false;
      if (nextVideo()) {
        nextVideoError = millis();
      }
    } else {
      showNoVideoError = true;
    }
  }

  //check for streaming data here?


  if (showNoVideoError) {
    displayNoVideosFound();
    delay(30);
    return;
  }

  // IR remote using NEC codes for next/previous channel, volume up and down, and mute
  IRInput();

  // This needs called all the time to consume potential serial data and switch to live mode
  streamer.fillBuffers(videoBuf[0], videoBuf[1], sizeof(videoBuf[0])/sizeof(videoBuf[0][0]));

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
      clearAudioBuffer();
      //memset(audioBuf, 127, AUDIOBUF_SIZE);
      //memcpy(audioBuf, shutoff, AUDIOBUF_SIZE);
      //sleep_ms(100);//wait to play?
      effects.startTurnOffEffect();
      //      digitalWrite(9, HIGH);
      //?digitalWrite(SPK_EN, LOW);
      clearDisplay();
    } else {
      TVscreenOffMode = false;
      if (doStaticEffects) effects.startChangeChannelEffect();
      playWhiteNoise = false;
      clearAudioBuffer();
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
      if (doStaticEffects && !streamer.live) effects.startChangeChannelEffect();
      if (nextVideo()) {
        nextVideoError = millis();
      } else {
        nextVideoError = 0;
      }
    }
  }
  if (channelDownInput) {
    channelDownInput = false;
    if (!TVscreenOffMode) {
      if (autoplayMode == 2) seekLivePos = true;
      if (doStaticEffects && !streamer.live) effects.startChangeChannelEffect();
      if (prevVideo()) {
        prevVideoError = millis();
      } else {
        prevVideoError = 0;
      }
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

  if(streamer.live){
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



  if (nextVideoError) {
    if ( millis() - nextVideoError < 3000) {
      displayPlaybackError(getCurrentFilename());
      delay(30);
    } else {
      nextVideoError = 0;
      channelUpInput = true;
    }
    return;
  } else if (prevVideoError) {
    if ( millis() - prevVideoError < 3000) {
      displayPlaybackError(getCurrentFilename());
      delay(30);
    } else {
      prevVideoError = 0;
      channelDownInput = true;
    }
    return;
  }






  if (isAVIStreamAvailable()) {
    uint32_t len = nextChunkLength();
    //cdc.println(len);
    if (len > 0) {
      if (isNextChunkAudio()) {
        // Found a chunk of audio, load it into the buffer
        uint8_t audioBuffer[512];
        int bytes = readNextChunk(audioBuffer, sizeof(audioBuffer));
        addToAudioBuffer(audioBuffer, bytes);
      } else if (isNextChunkVideo()) {
        // Found a chunk of video, decode it
        // Read the compressed JFIF data into the video buffer
        if (frameWaitDurationElapsed() && getFreeJPEGBuffer()) {
          readNextChunk(getFreeJPEGBuffer(), VIDEOBUF_SIZE);
          JPEGBufferFilled(len);
        }
      } else {
        cdc.print("chunk unrecognized ");
        cdc.println(len);
        if (autoplayMode != 0) {
          nextVideo();
        } else {
          startVideo("");
        }
      }

    } else {
      dbgPrint("0 length chunk or read error?");
      skipChunk();
      if (nextChunkLength() == 0) {
        dbgPrint("Two zero length chunks, skipping..");

        if (doStaticEffects) effects.startChangeChannelEffect();
        // Find a new video to play or loop
        if (autoplayMode != 0) {
          nextVideo();
        } else {
          startVideo("");
        }
      }
    }
  }

  uint64_t t1 = time_us_64();
  //dbgPrint("took " + String(uint32_t(t1 - t0)) + "us");

}

bool frameWaitDurationElapsed() {
  //cdc.print(audioSamplesInBuffer());
  if ((int64_t(time_us_64() - framerateHelper) < (targetFrameTime - 5000))) {
    delay(1);
    yield();
    return false;
  }
  //cdc.print(" ");
  //cdc.println(audioSamplesInBuffer());
  if (audioSamplesInBuffer() > 1000) {
    delay(1);
    yield();
    return false;
  }
  framerateHelper = time_us_64();
  return  true;
}


void setup1()
{

}

void loop1()
{
  core2Loop();
  MSCloopCore1();
}
