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
//  02/08/2023 Cross-platform base committed
//
//  Written by Mason Watmough, Ben Rose, and Jason Marcum for TinyCircuits, http://TinyCircuits.com
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
    the RP2040TV Player. If not, see <https://www.gnu.org/licenses/>  .
*/

// Select ONE from this list!

// #define TINYTV_2_COMPILE
// #define TINYTV_MINI_COMPILE
#define TINYTV_KIT_COMPILE

#ifdef TINYTV_KIT_COMPILE

  #define TinyTVKit 1
  #define TinyTVMini 1
  #define IR_INPUT_PIN 3

#elif defined(TINYTV_MINI_COMPILE)

  #define TinyTVMini 1
  #define IR_INPUT_PIN 1

#elif defined(TINYTV_2_COMPILE)

  #define IR_INPUT_PIN 1

#endif

// Uncomment to compile debug version
//#define DEBUGAPP (true)

// This include order matters
#include <SPI.h>

#include <TinyScreen.h>
#include <SdFat.h>                  // Custom SdFat for RP2040
#include "TinyIRReceiver.hpp"       // Unmodified IR library
#include <JPEGDEC.h>                // minor customization
#include "JPEGStreamer.h"
#include "globals_and_buffers.h"    // Big fat statically-allocate-everything header
#include "versions.h"
#ifndef TinyTVKit
#include "hardware/pwm.h"           // RP2040 API
#include "USB_MSC.h"                // Adafruit TinyUSB callbacks, and kinda hacky hathach tinyUSB start_stop_cb implementation
#endif






enum COMMAND_TYPE{
    NONE,
    FRAME_DELIMINATOR,
    TINYTV_TYPE
};

uint8_t commandCheck(uint8_t *jpegBuffer){
  // "0x30 0x30 0x64 0x63" is the start of an avi frame
  if(jpegBuffer[0] == 0x30 && jpegBuffer[1] == 0x30 && jpegBuffer[2] == 0x64 && jpegBuffer[3] == 0x63){
    //frameDeliminatorAcquired = true;
    return FRAME_DELIMINATOR;
    
  }else if(jpegBuffer[4] == 'T' && jpegBuffer[5] == 'Y' && jpegBuffer[6] == 'P' && jpegBuffer[7] == 'E'){
    #if !defined(TinyTVKit) && !defined(TinyTVMini)
      cdc.write("TV2");
    #elif !defined(TinyTVKit)
      cdc.write("TVMINI");
    #else
      SerialUSB.write("TVDIY");
    #endif
    return TINYTV_TYPE;
  }else if(jpegBuffer[5] == 'V' && jpegBuffer[6] == 'E' && jpegBuffer[7] == 'R'){
    // Allow for major.minor.patch up to [XXX.XXX.XXX]
    char version[12];
    sprintf(version, "[%u.%u.%u]", MAJOR, MINOR, PATCH);

    #if defined(TINYTV_2_COMPILE) || defined(TINYTV_MINI_COMPILE)
      cdc.write(version);
    #elif defined(TINYTV_KIT_COMPILE)
      SerialUSB.write(version);
    #endif
  }

  return NONE;
}


void commandSearch(uint8_t *jpegBuffer){
  #ifdef TinyTVKit
    while(SerialUSB.available()){
      // Move all bytes from right (highest index) to left (lowest index) in buffer
      jpegBuffer[0] = jpegBuffer[1];
      jpegBuffer[1] = jpegBuffer[2];
      jpegBuffer[2] = jpegBuffer[3];
      jpegBuffer[3] = jpegBuffer[4];
      jpegBuffer[4] = jpegBuffer[5];
      jpegBuffer[5] = jpegBuffer[6];
      jpegBuffer[6] = jpegBuffer[7];
      jpegBuffer[7] = SerialUSB.read();

      if(commandCheck(jpegBuffer) == FRAME_DELIMINATOR){
        break;
      }
    }
  #else
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
  #endif
}





int nextVideoError = 0;
int prevVideoError = 0;
bool showNoVideoError = false;
uint64_t TVscreenOffModeStartTime = 0;
extern TinyScreen display;

void setup() {
#ifdef DEBUGAPP
  cdc.println("test");
#endif
  if (!initPCIInterruptForTinyReceiver()) {
    dbgPrint("No interrupt available for IR_INPUT_PIN");
  }

#ifndef TinyTVKit // MSC time
  Serial.end();             // Called 'Serial' when on RP2040
  cdc.begin(0);
  USBMSCInit();
#else
  SerialUSB.begin(115200);  // Called 'SerialUSB' when on SAMD21
#endif

  yield();
  delay(100);
  initializeDisplay();
  initalizePins();
  // Do MSC setup quickly so PC recognizes us as a mass storage device
  if (!initializeSDcard()) {
    displayCardNotFound();
    while (1) {
    }
  }
#ifndef TinyTVKit
  USBMSCReady();
#endif
  if (!initializeFS()) {
    displayFileSystemError();
    while (1) {
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
#ifndef TinyTVKit // MSC loop

#endif
  nextVideo();
}
bool skipNextFrame = false;
unsigned long totalTime = 0;
void loop() {
  
#ifndef TinyTVKit // MSC loop
  
  if (USBJustConnected() && !live) {
    setAudioSampleRate(100);
    USBMSCStart();
    delay(50);
    displayUSBMSCmessage();

    if(TVscreenOffMode){
      TVscreenOffMode = false;
      // Turn backlight on
      #ifdef TinyTVMini
      #else
        digitalWrite(9, LOW);
        //backlightTurnedOff = false;
      #endif
    }
  }
  if(!live)
    if (handleUSBMSC(powerButtonPressed())) {
      //USBMSC active:
      return;
    }
  else if (!live && USBMSCJustStopped()) {
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
#endif

  if (showNoVideoError) {
    displayNoVideosFound();
    delay(30);
    return;
  }

  // IR remote using NEC codes for next/previous channel, volume up and down, and mute
  IRInput();

  #ifndef TinyTVKit
  if(getFreeJPEGBuffer() && incomingCDCHandler(getFreeJPEGBuffer(), VIDEOBUF_SIZE))
  {
    // ???
  }
  else
  {
    
  }
  #else
    commandSearch(videoBuf[0]);
  #endif
  // Hardware encoder/button input
  updateButtonStates();

  // Handle any input flags
  if(powerInput)
  {
    powerInput = false;
    if (doStaticEffects) changeChannelEffect();
    if (!TVscreenOffMode) powerDownTimer = micros() + 1000000/3;
    else powerDownTimer = micros() + 1000000/6;
  }
  if(powerDownTimer > micros())
  {
    if (powerDownTimer-2*targetFrameTime <= micros()) {
      powerDownTimer = micros();
      if (!TVscreenOffMode) {
        paused = true;
        TVscreenOffMode = true;
        TVscreenOffModeStartTime = millis();
        delay(30);
        clearAudioBuffer();
        while(!display.getReadyStatusDMA()) {}
        display.endTransfer();
        display.clearScreen();
        #ifdef TinyTVMini
        pauseRadius = 24;
        #else
        pauseRadius = 56;
        #endif
        while (pauseRadius > 3) tubeOffEffect();
        #ifndef TinyTVKit
          #ifdef TinyTVMini
            digitalWrite(9, LOW);
            display.off();
          #else
            digitalWrite(9, HIGH);
            display.off();
          #endif
        #else
        display.off();
        #endif
      } else {
        TVscreenOffMode = false;
        clearAudioBuffer();
        showChannelTimer = 120;
        paused = false;
        #ifndef TinyTVKit
        #ifdef TinyTVMini
        digitalWrite(9, HIGH);
        #else
        digitalWrite(9, LOW);
        #endif
        #endif
        display.on();
      }
    }
  }
  if (muteInput) {
    muteInput = false;
    if (!TVscreenOffMode) {
      setMute(!isMute());
    }
  }

  if (channelUpInput && !live) {
    channelUpInput = false;
    if (!TVscreenOffMode) {
      if (autoplayMode == 2) seekLivePos = true;
      if (doStaticEffects) changeChannelEffect();
      //nextVideoTimer = (1000000/targetFrameTime) / 6;
      //#endif
      if (nextVideo()) {
        nextVideoError = millis();
      } else {
        nextVideoError = 0;
      }
    }
  }
  if (channelDownInput && !live) {
    channelDownInput = false;
    if (!TVscreenOffMode) {
      if (autoplayMode == 2) seekLivePos = true;
      if (doStaticEffects) changeChannelEffect();
      //nextVideoTimer = -(1000000/targetFrameTime) / 6;
      //#endif
      if (prevVideo()) {
        prevVideoError = millis();
      } else {
        prevVideoError = 0;
      }
    }
  }
  /*
  if(nextVideoTimer < 0)
  {
    nextVideoTimer++;
  }
  if(nextVideoTimer > 0)
  {
    nextVideoTimer--;
    if(nextVideoTimer == 0)
    {
      if (nextVideo()) {
        nextVideoError = millis();
      } else {
        nextVideoError = 0;
      }
    }
  }
  */
  if (volUpInput && !live) {
    volUpInput = false;
    if (!TVscreenOffMode) {
      dbgPrint("v up");
      soundVolume += 32;
      if (soundVolume >= 256)
      {
      #ifndef TinyTVMini
        soundVolume = 256;
      #else
        soundVolume &= 0xFF; // Wrap audio around on the mini because we have no vol. down
      #endif
      }
      showVolumeTimer = 120;
    }
  }
  if (volDownInput && !live) {
    volDownInput = false;
    if (!TVscreenOffMode) {
      dbgPrint("v down");
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

  if(live) return;

  uint64_t t0 = micros();

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
    if (len > 0) {
      if (isNextChunkAudio()) {
        uint32_t t1 = micros();
        // Found a chunk of audio, load it into the buffer
        uint8_t audioBuffer[512];
        int bytes = readNextChunk(audioBuffer, sizeof(audioBuffer));
        addToAudioBuffer(audioBuffer, bytes);
        t1 = micros() - t1;

        totalTime += t1;
      } else if (isNextChunkVideo()) {
        // Found a chunk of video, decode it
        // Read the compressed JFIF data into the video buffer
        if (skipNextFrame) {
          skipChunk();
          skipNextFrame = false;
        } else if (frameWaitDurationElapsed() && getFreeJPEGBuffer()) {
          uint32_t t1 = micros();
          readNextChunk(getFreeJPEGBuffer(), VIDEOBUF_SIZE);
          JPEGBufferFilled(len);
          t1 = micros() - t1;

          totalTime += t1;

        }
      } else {
        dbgPrint("chunk unrecognized ");
        dbgPrint(String(len));
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

        if (doStaticEffects) changeChannelEffect();
        // Find a new video to play or loop
        if (autoplayMode != 0) {
          nextVideo();
        } else {
          startVideo("");
        }
      }
    }
  }


  unsigned long t1 = micros();
#ifdef TinyTVKit
  core2Loop();
#endif
  t1 = micros() - t1;

  if (t1 > 5000 && !live) {

    if (t1 + totalTime > targetFrameTime) {
      dbgPrint(String((uint32_t) (t1 + totalTime) - (uint32_t)targetFrameTime));
      dbgPrint(" ");
      dbgPrint(String(audioSamplesInBuffer()));
      if (audioSamplesInBuffer() < 200) {
        skipNextFrame = true;
        dbgPrint("Setting frameskip true, buffer is behind!");
      }
      else
      {
        skipNextFrame = false;
      }
    }
    totalTime = 0;
  }

  if (audioSamplesInBuffer() < 100) {
    dbgPrint(String(audioSamplesInBuffer()));
  }

}

bool frameWaitDurationElapsed() {
  if(live) return true;
  if ((int64_t(micros() - framerateHelper) < (targetFrameTime - 5000))) {
    delay(1);
    yield();
    return false;
  }
  if (audioSamplesInBuffer() > 1000) {
    delay(1);
    yield();
    return false;
  }
  framerateHelper = micros();
  return  true;
}


#ifndef TinyTVKit
void setup1()
{
  initializeDisplay();
}

void loop1()
{
  core2Loop();
}
#endif
