//-------------------------------------------------------------------------------
//  TinyCircuits TinyTV Firmware
//
//  Board Package: Raspberry Pi Pico/RP2040 by Earle F. Philhower, III version 2.6.0
//  Board: Raspberry Pi Pico
//  CPU Speed: 200MHz TinyTV 2/ 50MHZ TinyTV Mini
//  USB Stack: Adafruit TinyUSB
//
//  Changelog:
//  05/26/2023 Initial Release for TinyTV 2/Mini
//  02/08/2023 Cross-platform base committed
//
//  Board Package customizations:
//  packages\rp2040\hardware\rp2040\2.5.2\variants\rpipico  ->  #define PIN_LED        (12u)
//  packages\rp2040\hardware\rp2040\2.5.2\libraries\Adafruit_TinyUSB_Arduino\src\arduino\ports\rp2040 tweaked CFG_TUD_MSC_EP_BUFSIZE to 512*8
//
//  Written by Mason Watmough, Ben Rose, and Jason Marcum for TinyCircuits, http://TinyCircuits.com
//
//-------------------------------------------------------------------------------

/*
    This file is part of the TinyCircuits TinyTV Firmware.
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

// Uncomment to compile debug version
//#define DEBUGAPP (true)

// This include order matters
#include <SPI.h>
#include "src/TinyScreen/TinyScreen.h"
#include <SdFat.h>
#include <JPEGDEC.h>                // minor customization
#include "globals.h"
#include "versions.h"

SdFat32 sd;
File32 infile;
File32 dir;
JPEGDEC jpeg;

#ifdef ARDUINO_ARCH_RP2040
#include <Adafruit_TinyUSB.h>
Adafruit_USBD_MSC usb_msc;
Adafruit_USBD_CDC cdc;
#include "USB_MSC.h"                // Adafruit TinyUSB callbacks, and kinda hacky tinyUSB start_stop_cb implementation, thanks hathach!
#define has_USB_MSC 1
#endif
#include "USB_CDC.h"


// Select ONE from this list!
#include "TinyTV2.h"
//#include "TinyTVMini.h"
//#include "TinyTVKit.h"


#include "videoBuffer.h"
#include "settings.h"
#include "TinyIRReceiver.hpp"       // Unmodified IR library- requires IR_INPUT_PIN defined in hardware header





// Local playback vars
int nextVideoError = 0;
int prevVideoError = 0;
bool showNoVideoError = false;
uint64_t TVscreenOffModeStartTime = 0;
bool skipNextFrame = false;
unsigned long totalTime = 0;
uint64_t powerDownTimer = 0;
bool TVscreenOffMode = false;
uint32_t settingsNeedSaved = 0;
uint64_t framerateHelper = 0;
bool live = false;
int staticTimeMS = 300;

void setup() {
#ifdef has_USB_MSC
  Serial.end();
  cdc.begin(0);
  // Do MSC setup quickly so PC recognizes us as a mass storage device
  USBMSCInit();
  yield();
  delay(100);
  yield();
#endif
  //SerialUSB.begin(9600);
  //while(!SerialUSB);
  clearAudioBuffer();
  initializeInfrared();
  initializeDisplay();
  initalizePins();

  if (!initializeSDcard()) {
    displayCardNotFound();
    while (1) {
      uint16_t totalJPEGBytesUnused;
      incomingCDCHandler(getFreeJPEGBuffer(), /*VIDEOBUF_SIZE*/0, &live, &totalJPEGBytesUnused);
#ifndef TinyTVKit
      if (powerButtonPressed())
        hardwarePowerOff();
#endif
    }
  }

  // Finish USB MSC setup once card capacity is known
#ifdef has_USB_MSC
  USBMSCReady();
#endif

  if (!initializeFS()) {
    displayFileSystemError();
    while (1) {
      uint16_t totalJPEGBytesUnused;
      incomingCDCHandler(getFreeJPEGBuffer(), /*VIDEOBUF_SIZE*/0, &live, &totalJPEGBytesUnused);
#ifndef TinyTVKit
      if (powerButtonPressed())
        hardwarePowerOff();
#endif
    }
  }

  initVideoPlayback();
}

void initVideoPlayback() {
  loadSettings();
  setVolume(volumeSetting);
  if (loadVideoList()) {
    if (startVideoByChannel(channelNumber)) {
      nextVideoError = millis();
    } else {
      //prevVideo();
      drawChannelNumberFor(1000);
      setAudioSampleRate(getVideoAudioRate());
      clearAudioBuffer();
      drawStaticFor(staticTimeMS);
      playStaticFor(staticTimeMS);
    }
  } else {
    showNoVideoError = true;
  }
}

void loop() {

#ifdef has_USB_MSC
  if (USBJustConnected() && !live) {
    setAudioSampleRate(100);
    USBMSCStart();
    for (int i = 0; i < 50; i++) {
      delay(1); yield();
    }
    if (TVscreenOffMode) {
      TVscreenOffMode = false;
      displayOn();
    }
    displayUSBMSCmessage();
  }
  if (!live) {
    if (handleUSBMSC(powerButtonPressed())) {
      //USBMSC active, handle CDC commands except for filling frames:
      uint16_t totalJPEGBytesUnused;
      incomingCDCHandler(getFreeJPEGBuffer(), /*VIDEOBUF_SIZE*/0, &live, &totalJPEGBytesUnused);
      return;
    } //else
    if (USBMSCJustStopped()) {
      for (int i = 0; i < 50; i++) {
        delay(1); yield();
      }
      //USBMSC ejected, return to video playback:
      clearPowerButtonPressInt();
      initVideoPlayback();
    }
  }
#endif


  if (getFreeJPEGBuffer()) {
    uint16_t totalJPEGBytes = 0;
    if (incomingCDCHandler(getFreeJPEGBuffer(), VIDEOBUF_SIZE, &live, &totalJPEGBytes)) {
      // new frame
      JPEGBufferFilled(totalJPEGBytes);
    }
  }
  if (inputFlags.settingsChanged) {
    inputFlags.settingsChanged = false;
    settingsNeedSaved = millis();
  }


  // IR remote using NEC codes for next/previous channel, volume up and down, and mute
  IRInput(&inputFlags);

  // Hardware encoder/button input
  updateButtonStates(&inputFlags);

  // Handle any input flags

  if (inputFlags.power) {
    inputFlags.power = false;
    if (doStaticEffects) {
      drawStaticFor(staticTimeMS);
      playStaticFor(staticTimeMS);
    }
    if (TVscreenOffMode) {
      //on
      TVscreenOffMode = false;
      clearAudioBuffer();
      //drawChannelNumberFor(1000);
      displayOn();
    } else {
      //set off timer
      powerDownTimer = millis();
    }
  }

  if (powerDownTimer && millis() - powerDownTimer > staticTimeMS) {
    //off
    powerDownTimer = 0;
    TVscreenOffMode = true;
    TVscreenOffModeStartTime = millis();
    //delay(30);
    clearAudioBuffer();
    clearDisplay();
    startTubeOffEffect();
    while (tubeOffEffect() > 3);
    //stopStaticEffect();
    displayOff();
  }

  if (TVscreenOffMode) {
#ifndef TinyTVKit
    // Turn TV off after 2 minutes in screen off mode
    if (millis() - TVscreenOffModeStartTime > 1000 * 60 * 2) {
      hardwarePowerOff();
    }
#endif
    return;
  }

  if (inputFlags.mute) {
    inputFlags.mute = false;
    if (!TVscreenOffMode) {
      setMute(!isMute());
    }
  }

  if (inputFlags.channelUp) {
    inputFlags.channelUp = false;
    if (!TVscreenOffMode && !live) {
      settingsNeedSaved = millis();
      if (doStaticEffects) {
        drawStaticFor(staticTimeMS);
        playStaticFor(staticTimeMS);
      }
      if (nextVideo()) {
        nextVideoError = millis();
      } else {
        nextVideoError = 0;
        setAudioSampleRate(getVideoAudioRate());
        drawChannelNumberFor(1000);
      }
    }
  }

  if (inputFlags.channelDown) {
    inputFlags.channelDown = false;
    if (!TVscreenOffMode && !live) {
      settingsNeedSaved = millis();
      if (doStaticEffects) {
        drawStaticFor(staticTimeMS);
        playStaticFor(staticTimeMS);
      }
      if (prevVideo()) {
        prevVideoError = millis();
      } else {
        prevVideoError = 0;
        setAudioSampleRate(getVideoAudioRate());
        drawChannelNumberFor(1000);
      }
    }
  }
  if (inputFlags.channelSet) {
    inputFlags.channelSet = false;
    if (!TVscreenOffMode && !live) {
      settingsNeedSaved = millis();
      if (doStaticEffects) {
        drawStaticFor(staticTimeMS);
        playStaticFor(staticTimeMS);
      }
      if (startVideoByChannel(channelNumber)) {
        nextVideoError = millis();
      } else {
        nextVideoError = 0;
        setAudioSampleRate(getVideoAudioRate());
        drawChannelNumberFor(1000);
      }
    }
  }

  if (inputFlags.volUp) {
    inputFlags.volUp = false;
    if (!TVscreenOffMode && !live) {
      settingsNeedSaved = millis();
      dbgPrint("vol up");
      volumeUp();
      drawVolumeFor(1000);
    }
  }
  if (inputFlags.volDown) {
    inputFlags.volDown = false;
    if (!TVscreenOffMode && !live) {
      settingsNeedSaved = millis();
      dbgPrint("vol down");
      volumeDown();
      drawVolumeFor(1000);
    }
  }
  if (inputFlags.volumeSet) {
    inputFlags.volumeSet = false;
    if (!TVscreenOffMode && !live) {
      setVolume(volumeSetting);
      settingsNeedSaved = millis();
      drawVolumeFor(1000);
    }
  }



  if (live) {
#ifdef TinyTVKit
    loop1();
#endif
    return;
  }

  uint64_t t0 = micros();

  if (showNoVideoError) {
    displayNoVideosFound();
    delay(30);
  } else if (nextVideoError) {
    if ( millis() - nextVideoError < 3000) {
      displayPlaybackError(getCurrentFilename());
      delay(30);
    } else {
      nextVideoError = 0;
      inputFlags.channelUp = true;
    }
    return;
  } else if (prevVideoError) {
    if ( millis() - prevVideoError < 3000) {
      displayPlaybackError(getCurrentFilename());
      delay(30);
    } else {
      prevVideoError = 0;
      inputFlags.channelDown = true;
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
        if (loopVideo == false) {
          nextVideo();
          setAudioSampleRate(getVideoAudioRate());
          drawChannelNumberFor(1000);
        } else {
          startVideo("", 0);
        }
      }

    } else {
      dbgPrint("0 length chunk or read error?");
      skipChunk();
      if (nextChunkLength() == 0) {
        dbgPrint("Two zero length chunks, skipping..");

        if (doStaticEffects) {
          drawStaticFor(staticTimeMS);
          playStaticFor(staticTimeMS);
        }
        // Find a new video to play or loop
        if (loopVideo == false) {
          nextVideo();
          setAudioSampleRate(getVideoAudioRate());
          drawChannelNumberFor(1000);
        } else {
          startVideo("", 0);
        }
      }
    }
  }


  unsigned long t1 = micros();
#ifdef TinyTVKit
  loop1();
#endif
  t1 = micros() - t1;

  if (t1 > 5000 && !live) {//kit only

    if (t1 + totalTime > targetFrameTime) {
      //dbgPrint(String((uint32_t) (t1 + totalTime) - (uint32_t)targetFrameTime));
      //dbgPrint(" ");
      //dbgPrint(String(audioSamplesInBuffer()));
      if (getVideoAudioRate() && audioSamplesInBuffer() < 200) {
        skipNextFrame = true;
        dbgPrint("Setting frameskip true, buffer is behind!");
      } else {
        skipNextFrame = false;
      }
    }
    totalTime = 0;
  }
#ifndef TinyTVKit
  if (!live) {
    if (getVideoAudioRate() && audioSamplesInBuffer() < 200) {
      skipNextFrame = true;
      dbgPrint("Setting frameskip true, buffer is behind!");
    } else {
      skipNextFrame = false;
    }
  }
#endif
  if (getVideoAudioRate() && audioSamplesInBuffer() < 100) {
    dbgPrint(String(audioSamplesInBuffer()));
  }


  if (settingsNeedSaved) {
    if (millis() - settingsNeedSaved > 2000) {
      dbgPrint("Saving settings file");
      saveSettings();
      settingsNeedSaved = 0;
      dbgPrint("Saved settings file");
    }
  }


}

bool frameWaitDurationElapsed() {
  if (live) return true;
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


void setup1() {
  //initializeDisplay();
}

void loop1() {
  if (TVscreenOffMode) {
    return;
  }
  //decode JPEG if available
  if (!getFilledJPEGBuffer()) {
    return;
  }
  //streamer.decode(getFilledJPEGBuffer(), getJPEGBufferLength(), JPEGDraw);

  if (!jpeg.openRAM(getFilledJPEGBuffer(), getJPEGBufferLength(), JPEGDraw)) {
    if (getJPEGBufferLength() == 240) {
      //probably a blank frame
    } else {
      dbgPrint("Could not open frame from RAM! Error: ");
      dbgPrint(String(jpeg.getLastError()));
      dbgPrint("See https://github.com/bitbank2/JPEGDEC/blob/master/src/JPEGDEC.h#L83");
    }
  }
  newJPEGFrameSize(jpeg.getWidth(), jpeg.getHeight());
  jpeg.setPixelType(RGB565_BIG_ENDIAN);
  jpeg.setMaxOutputSize(2048);
  jpeg.decode(0, 0, 0);

  JPEGBufferDecoded();
}
