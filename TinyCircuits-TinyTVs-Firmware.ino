//-------------------------------------------------------------------------------
//  TinyCircuits TinyTV Firmware
//
//  Board Package: Raspberry Pi Pico/RP2040 by Earle F. Philhower, III version 2.6.0
//  Board: Raspberry Pi Pico
//  CPU Speed: 200MHz TinyTV 2/ 50MHZ TinyTV Mini
//  USB Stack: Adafruit TinyUSB
//
//  Changelog:
//  03/25/2026 MP4 playback update
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

bool streamError = false;

// Select ONE from this list!
#include "TinyTV2.h"
//#include "TinyTVMini.h"
//#include "TinyTVKit.h"

#ifdef ARDUINO_ARCH_RP2040
#include <Adafruit_TinyUSB.h>
Adafruit_USBD_MSC usb_msc;
Adafruit_USBD_CDC cdc;
#include "USB_MSC.h"                // Adafruit TinyUSB callbacks, and kinda hacky tinyUSB start_stop_cb implementation, thanks hathach!
#define has_USB_MSC 1
#endif
#include "USB_CDC.h"

#include "videoBuffer.h"
#include "settings.h"



// Local playback vars
int nextVideoError = 0;
int prevVideoError = 0;
bool showNoVideoError = false;
uint64_t TVscreenOffModeStartTime = 0;
bool skipNextFrame = false;
unsigned long totalTime = 0;
uint64_t powerDownTimer = 0;
bool TVscreenOffMode = false;
uint64_t settingsNeedSaved = 0;
uint64_t framerateHelper = 0;
bool live = false;
int staticTimeMS = 300;
char splashVidFileName[20] = "";
bool splashPlaybackMode = false;
bool firstFrame = true;
bool wasInit = false;
bool lastH264Sample = false;

#ifndef TinyTVKit

#ifdef __cplusplus
extern "C" {
#include "src/h264bsd/basetype.h"
#include "src/h264bsd/h264bsd_decoder.h"
#include "src/h264bsd/h264bsd_util.h"

#include "src/foxen-flac/foxen-flac.h"
}
#endif

storage_t *decHd;
fx_flac_t *decFlac = NULL;

#endif

extern "C" {
#include "TinyIRReceiver.hpp"       // Unmodified IR library- requires IR_INPUT_PIN defined in hardware header
}

// #ifndef TinyTVKit

// extern char __StackLimit, __bss_end__;

// // Helper to get total heap size (available at compile time)
// uint32_t _getTotalHeap() {
//     return &__StackLimit - &__bss_end__;
// }

// // Helper to get current free heap size
// uint32_t getFreeHeap() {
//     struct mallinfo m = mallinfo();
//     return _getTotalHeap() - m.uordblks;
// }

// #endif

void setup() {
#ifdef has_USB_MSC
  USBMSC_bus_detach_before_rebuild();
  Serial.end();
  cdc.begin(0);
  // Do MSC setup quickly so PC recognizes us as a mass storage device
  USBMSCInit();
  yield();
  delay(100);
  yield();
#endif
  clearAudioBuffer();
  initializeInfrared();
  initializeDisplay();
  initalizePins();
  //dbgPrint("Initialized HW");

  if (!initializeSDcard()) {
#ifdef has_USB_MSC
    // Happy path calls this after USBMSCReady(); without SD we never get there — still
    // attach so CDC works for the error loop.
    USBMSC_bus_attach_after_msc_geometry_ready();
#endif
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
  cdc.begin(115200);
  USBMSC_bus_attach_after_msc_geometry_ready();
#endif
  
#ifndef TinyTVKit
  decHd = h264bsdAlloc();
  if (decHd != NULL) {
    dbgPrint("SUCCESS: Allocated decoder storage structure!\n");
  } else {
    dbgPrint("ERROR: Could not allocate decoder storage stucture!\n");
  }

  decFlac = FX_FLAC_ALLOC(FLAC_SUBSET_MAX_BLOCK_SIZE_48KHZ, 1U);
  if(!decFlac) {
    dbgPrint("ERROR: Could not initialize FLAC decoder!");
  } else {
    dbgPrint("SUCCESS: Initialized FLAC decoder!");
  }

  initFlacMtx();

#endif

  dbgPrint("Starting playback!");

  initVideoPlayback(true);
}

void initVideoPlayback(bool loadSettingsFile) {
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
  if (loadSettingsFile) {
    if(LittleFS.exists("settings.txt")) {
      loadSettingsFlashBuffer();
    } else {
      loadSettings();
    }
  }
  randomSeed(micros());
  if (randStartTime) {
    setMillisOffset(random(1000000) * 1000);
  } else {
    setMillisOffset(0);
  }
  setVolume(volumeSetting);
  int videoCount = loadVideoList(splashVidFileName);
  if (videoCount) {
    showNoVideoError = false;
    if (randStartChan) {
      channelNumber = random(videoCount) + 1;
    }
    if (strlen(splashVidFileName)) {
      dbgPrint(splashVidFileName);
      splashPlaybackMode = true;
      startVideo(splashVidFileName, 0);
      setAudioSampleRate(getVideoAudioRate());
      clearAudioBuffer();
      inputFlags.channelSet = false; //Stop channel from changing after reading settings file
      inputFlags.volumeSet = false; //Stop volume from changing after reading settings file
    } else {
      if (startVideoByChannel(channelNumber)) {
        nextVideoError = millis();
      } else {
        drawChannelNumberFor(1000);
        setAudioSampleRate(getVideoAudioRate());
        clearAudioBuffer();
        drawStaticFor(staticTimeMS);
        playStaticFor(staticTimeMS);
      }
    }
  } else {
    showNoVideoError = true;
  }
}

void loop() {
#ifdef has_USB_MSC
  if (USBJustConnected() && !live) {
    releaseSdCardForUSBMSC();
    setAudioSampleRate(100);
    dbgPrint("Saving settings to SD card!");
    saveSettings(); // Save settings to SD card on USB connect
    dbgPrint("Saved!");
    USBMSCStart();
    for (int i = 0; i < 50; i++) {
      delay(1);
#ifdef ARDUINO_ARCH_RP2040
      msc_yield_usb_only();
#else
      yield();
#endif
    }
    if (TVscreenOffMode) {
      TVscreenOffMode = false;
      displayOn();
    }
    displayUSBMSCmessage();
  }
  if (handleUSBMSC(powerButtonPressed())) {
    // Do not run incomingCDCHandler() during MSC (see USB_MSC / MACOS_USB_STORAGE).
    // Do not use yield() here: it flushes CDC TX every time and breaks MSC on macOS composite.
#ifdef ARDUINO_ARCH_RP2040
    msc_yield_usb_only();
#else
    yield();
#endif

    // Do CDC things anyways
    uint16_t totalJPEGBytesUnused;
    if (getFreeJPEGBuffer()) {
      if (incomingCDCHandler(getFreeJPEGBuffer(), VIDEOBUF_SIZE, &live, &totalJPEGBytesUnused)) {
        handleUSBMSC(true);
      }
    } else {
      incomingCDCHandler(NULL, 0, &live, &totalJPEGBytesUnused);
    }

    return;
  }
  if (USBMSCJustStopped()) {
    dbgPrint("MSC Stopped");
    for (int i = 0; i < 50; i++) {
      delay(1);
#ifdef ARDUINO_ARCH_RP2040
      msc_yield_usb_only();
#else
      yield();
#endif
    }
    //USBMSC ejected, return to video playback:
    clearPowerButtonPressInt();

    if (inputFlags.settingsChanged) {
      inputFlags.settingsChanged = false;
      dbgPrint("Overwriting settings to LFS!");
      saveSettingsFlashBuffer();
      saveSettings();
      dbgPrint("Overwritten!");
    } else {
      dbgPrint("Copying settings to LFS!");

      File32 src;
      src.open("settings.txt", O_READ);

      File dst = LittleFS.open("settings.txt", "w");
      uint8_t buf[512];
      
      while(src.available()) {
        size_t read = src.read(buf, 512);
        dst.write(buf, read);
      }

      dst.close();
      src.close();

      dbgPrint("Copied!");
    }
    initVideoPlayback(true);
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
    dbgPrint("inputFlags.settingsChanged");
    inputFlags.settingsChanged = false;
    settingsNeedSaved = millis();
  }


  // IR remote using NEC codes for next/previous channel, volume up and down, and mute
  IRInput(&inputFlags);

  // Hardware encoder/button input
  updateButtonStates(&inputFlags);

  // Handle any input flags

  if (inputFlags.power) {
    dbgPrint("inputFlags.power");
    inputFlags.power = false;
    if (doStaticEffects) {
      drawStaticFor(staticTimeMS);
      playStaticFor(staticTimeMS);
    }
    if (TVscreenOffMode) {
      //on
      TVscreenOffMode = false;
      clearAudioBuffer();
      displayOn();
      if (strlen(splashVidFileName)) {
        dbgPrint(splashVidFileName);
        splashPlaybackMode = true;
        startVideo(splashVidFileName, 0);
        setAudioSampleRate(getVideoAudioRate());
        clearAudioBuffer();
      }
    } else if (!powerDownTimer) {
      //set off timer
      powerDownTimer = millis();
    }
  }

  if (powerDownTimer && millis() - powerDownTimer > staticTimeMS) {
    if (settingsNeedSaved) saveSettingsFlashBuffer();
    //off
    powerDownTimer = 0;
    TVscreenOffMode = true;
    TVscreenOffModeStartTime = millis();
    while(!getFreeJPEGBuffer()) {yield();}
    //while(getFilledJPEGBuffer()) {yield();}
    delay(30);//allow any frames to be displayed
    resetBuffers();
    clearAudioBuffer();
    
    clearDisplay();
    startTubeOffEffect();
    while (tubeOffEffect() > 3);
    displayOff();
  }

  if (inputFlags.mute) {
    dbgPrint("inputFlags.mute");
    inputFlags.mute = false;
    if (!TVscreenOffMode) {
      setMute(!isMute());
    }
  }

  if (inputFlags.channelUp) {
    dbgPrint("inputFlags.channelUp");
    inputFlags.channelUp = false;
    splashPlaybackMode = false;
    if (!TVscreenOffMode && !live) {
      settingsNeedSaved = millis();
      if (nextVideo()) {
        nextVideoError = millis();
      } else {
        nextVideoError = 0;
        if (doStaticEffects) {
          drawStaticFor(staticTimeMS);
          playStaticFor(staticTimeMS);
        }
        setAudioSampleRate(getVideoAudioRate());
        drawChannelNumberFor(1000);
      }
    }
  }

  if (inputFlags.channelDown) {
    dbgPrint("inputFlags.channelDown");
    inputFlags.channelDown = false;
    splashPlaybackMode = false;
    if (!TVscreenOffMode && !live) {
      settingsNeedSaved = millis();
      if (prevVideo()) {
        prevVideoError = millis();
      } else {
        prevVideoError = 0;
        if (doStaticEffects) {
          drawStaticFor(staticTimeMS);
          playStaticFor(staticTimeMS);
        }
        setAudioSampleRate(getVideoAudioRate());
        drawChannelNumberFor(1000);
      }
    }
  }
  
  if (inputFlags.channelSet) {
    dbgPrint("inputFlags.channelSet");
    inputFlags.channelSet = false;
    splashPlaybackMode = false;
    if (!TVscreenOffMode && !live) {
      settingsNeedSaved = millis();
      if (startVideoByChannel(channelNumber)) {
        nextVideoError = millis();
      } else {
        nextVideoError = 0;
        if (doStaticEffects) {
          drawStaticFor(staticTimeMS);
          playStaticFor(staticTimeMS);
        }
        setAudioSampleRate(getVideoAudioRate());
        drawChannelNumberFor(1000);
      }
    }
  }

  if (inputFlags.volUp) {
    dbgPrint("inputFlags.volUp");
    inputFlags.volUp = false;
    if (!TVscreenOffMode && !live) {
      settingsNeedSaved = millis();
      dbgPrint("vol up");
      volumeUp();
      drawVolumeFor(1000);
    }
  }
  if (inputFlags.volDown) {
    dbgPrint("inputFlags.volDown");
    inputFlags.volDown = false;
    if (!TVscreenOffMode && !live) {
      settingsNeedSaved = millis();
      dbgPrint("vol down");
      volumeDown();
      drawVolumeFor(1000);
    }
  }
  if (inputFlags.volumeSet) {
    dbgPrint("inputFlags.volumeSet");
    inputFlags.volumeSet = false;
    if (!TVscreenOffMode && !live) {
      setVolume(volumeSetting);
      settingsNeedSaved = millis();
      drawVolumeFor(1000);
    }
  }
  
  if (TVscreenOffMode) {
#ifndef TinyTVKit
    // Turn TV off after specified duration in screen off mode
    if (millis() - TVscreenOffModeStartTime > 1000 * powerTimeoutSecs) {
      hardwarePowerOff();
    }
#endif
    return;
  }

  if (live) {
#ifdef TinyTVKit
    loop1();
#endif
    return;
  }
  
  uint64_t t1 = micros();

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

  streamError = false;
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
        // Read the compressed JFIF data into the video buffer
        if (skipNextFrame) {
          dbgPrint("Skipping AVI frame!");
          skipChunk();
          skipNextFrame = false;
        } else if (frameWaitDurationElapsed() && getFreeJPEGBuffer()) {
          uint32_t t1 = micros();
          readNextChunk(getFreeJPEGBuffer(), VIDEOBUF_SIZE);
          JPEGBufferFilled(len);
          t1 = micros() - t1;
          totalTime += t1;
        }
      } else if (isNextChunkIndex()) {
        if (jumpToNextMoviList()) {
          dbgPrint("jumpToNextMoviList error");
          streamError = true;
        }
      } else {
        dbgPrint("chunk unrecognized ");
        dbgPrint(String(len));
        streamError = true;
      }
    } else {
      dbgPrint("0 length chunk or read error?");
      skipChunk();
      if (nextChunkLength() == 0) {
        dbgPrint("Two zero length chunks, skipping..");
        streamError = true;
      }
    }
  } else if (isTSVStreamAvailable()) {
    if (frameWaitDurationElapsed()) {
      newJPEGFrameSize(VIDEO_W, VIDEO_H);
      dbgPrint("Set frame size!");
      JPEGDRAW jd;
      jd.x = 0;
      jd.y = 0;
      jd.iWidth = VIDEO_W;
      jd.iHeight = 16;
      #ifdef TinyTVKit
      jd.pPixels = (uint16_t*)(videoBuf[0]);
      #else
      jd.pPixels = getFrameBuffer();
      #endif
      int totalHeight = VIDEO_H;
      while (totalHeight && !streamError) {
        int blockHeight = min(16, totalHeight);
        dbgPrint("Reading TSV data...");
        int bytes = readTSVBytes((uint8_t*)(jd.pPixels), VIDEO_W * blockHeight * 2);
        dbgPrint("Read TSV data, "+String(bytes)+" bytes, totalHeight="+String(totalHeight));
        if (bytes == VIDEO_W * blockHeight * 2) {
          uint16_t bgr;
          for (int i = 0; i < VIDEO_W * blockHeight; i++) {
            bgr = ((uint16_t*)(jd.pPixels))[i];
            bgr = ((bgr & 0x00ff) << 8) | ((bgr & 0xff00) >> 8);
            bgr = ((bgr << 11) & 0xF800) | (bgr & 0x07E0) | ((bgr >> 11) & 0x001F);
            bgr = ((bgr & 0x00ff) << 8) | ((bgr & 0xff00) >> 8);
            ((uint16_t*)(jd.pPixels))[i] = bgr;
          }
          jd.iHeight = blockHeight;
          JPEGDraw(&jd);
          dbgPrint("Drew TSV data at y="+String(jd.y)+", x="+String(jd.x)+", width="+String(jd.iWidth)+", height="+String(jd.iHeight));
          jd.y += blockHeight;
          totalHeight -= blockHeight;
        } else {
          dbgPrint("TSV video data read error");
          streamError = true;
        }
      }
      for (int blocks = 0; (blocks < 4) && !streamError; blocks++) {
        uint8_t audioBuffer[512];
        dbgPrint("Reading TSV audio data...");
        int bytes = readTSVBytes(audioBuffer, sizeof(audioBuffer));
        if (bytes == sizeof(audioBuffer)) {
          for (int i = 0; i < bytes / 2; i++) {
            uint16_t sample = ((uint16_t *)audioBuffer)[i];
            audioBuffer[i] = sample >> 2;
          }
          addToAudioBuffer(audioBuffer, bytes / 2);
        } else {
          dbgPrint("TSV audio data read error");
          streamError = true;
        }
      }
    }
  } 
  #ifndef TinyTVKit
  else if(isMP4StreamAvailable()) {
    
    if (getMP4ToplevelError()) {
      resetMP4ToplevelError();
      dbgPrint("getMP4Toplevel error");
      streamError = true;
    }

    loadNextTraf();

    if(!getH264DecodeReady() && getH264Ready()) {
        
      if(getH264TrafCurrentSample() < (getH264SampleCount())) {
        if(getH264TrafCurrentSample() == (getH264SampleCount()-1)) {
          dbgPrint("Buffering last H264 sample!!");
          //while(lastH264Sample) { yield(); }
          // loadLastH264Sample(getH264TrafCurrentSample());
          // pushLastH264Sample(getFreeJPEGBuffer());
          lastH264Sample = true;
          int bufSize;
          uint8_t* src = getH264SamplePtr(getH264TrafCurrentSample(), &bufSize);
          dbgPrint("buffer size: "+String(bufSize));
          memcpy(getH264LastSampleBuf(), src, bufSize);
          setH264LastSampleSize(bufSize);
          H264Used();
          resetH264Traf();
        } else {
          int s_size;
          getH264Sample(getFreeJPEGBuffer(), getH264TrafCurrentSample(), &s_size);
          advanceH264TrafCurrentSample();
          while(!frameWaitDurationElapsed()) { loadFLACDataChunk(); }
        }

        //getH264Sample(getFreeJPEGBuffer(), getH264TrafCurrentSample(), &s_size);
        
        dbgPrint("Marking buffer filled!");
        setH264DecodeReady();
      
      } else {
        H264Used();
        resetH264Traf();
      }
    } else if(!getH264DecodeReady() && (getH264TrafCurrentSample() >= (getH264SampleCount()))) {
        H264Used();
        resetH264Traf();
    } else {
      //dbgPrint("MP4 decode loop spinning...");
    }
  }
  #endif 
  else {
    dbgPrint("No stream!!");
  }

  if (streamError) {
    dbgPrint("streamError");
    if (splashPlaybackMode) {
      dbgPrint("streamError splashPlaybackMode");
      inputFlags.channelSet = true;
      splashPlaybackMode = false;
    } else {
      // Find a new video to play or loop
      if (loopVideo == false) {
        inputFlags.channelUp = true;
      } else {
        inputFlags.channelSet = true;
      }
    }
  }
  

#ifdef TinyTVKit
  loop1();
#endif
  t1 = micros() - t1;

  if (t1 > 5000 && !live) {//kit only

    if (t1 + totalTime > (targetFrameTime)) {
      if (getVideoAudioRate() && audioSamplesInBuffer() < 200) {
        skipNextFrame = true;
        //dbgPrint("Setting frameskip true, buffer is behind!");
      } else {
        skipNextFrame = false;
      }
    }
    totalTime = 0;
  }
#ifndef TinyTVKit
  if (!live) {
    skipNextFrame = false;
    if (getVideoAudioRate() && getAudioSampleCount() < 50) {
      skipNextFrame = true;
      //dbgPrint("Setting frameskip true, buffer is behind!");
    }
  }
#endif
  if (getVideoAudioRate() && getAudioSampleCount() < 100) {
    //dbgPrint(String(audioSamplesInBuffer()));
  }

  if (settingsNeedSaved) {
    if (millis() - settingsNeedSaved > 2000) {
      dbgPrint("Saving settings file");
      saveSettingsFlashBuffer();
      settingsNeedSaved = 0;
      dbgPrint("Saved settings file");
    }
  }
}

bool frameWaitDurationElapsed() {
  if (live) return true;
  if(!isMP4StreamAvailable()) {
    if ((int64_t(micros() - framerateHelper) < (targetFrameTime - 5000))) {
      dbgPrint("frame wait");
      delay(1);
      yield();
      return false;
    }
    if ((getAudioSampleCount() > AUDIOBUF_SIZE-1000)) {
      dbgPrint("audio wait");
      delay(1);
      yield();
      return false;
    }
  } else {
    if ((int64_t(micros() - framerateHelper) < (targetFrameTime - 5000))) {
      yield();
#ifndef TinyTVKit
      // dbgPrint("Loading FLAC chunks inner, framerateHelper: "+String((int)framerateHelper)+", diff: "+String((int)(int64_t(micros() - framerateHelper))));
      // dbgPrint("targetFrameTime: "+String((int)(targetFrameTime - 5000)));
      loadFLACDataChunk();
#endif
      return false;
    }
  }
  framerateHelper = micros();
  return  true;
}

#ifndef TinyTVKit
__attribute__((aligned(4))) uint16_t singleLineBuf[2][4096];
int lineBufIdx = 0;
uint8_t* oldFramePtr = NULL;
int totalDroppedFrames = 0;
#endif

void setup1() {
  
}

void loop1() {

  #if defined(has_USB_MSC) && !defined(TinyTVKit)

  MSCloopCore1();

  #endif
  
  if (TVscreenOffMode) {
    return;
  }
  
  //decode JPEG if available
  #ifndef TinyTVKit
  if (!getFilledJPEGBuffer() && !getH264DecodeReady()) {
    //dbgPrint("No filled JPEG buffer or H264 data!");
    return;
  }
  #else
  if (!getFilledJPEGBuffer()) {
    return;
  }
  #endif

  if(isMP4StreamAvailable() && !wasInit) {
    dbgPrint("No decoder!");
    return;
  }
  
  #ifndef TinyTVKit
  #endif

  uint64_t t0 = micros();
  
  #ifndef TinyTVKit
  if(isMP4StreamAvailable()) {

    #ifdef TinyTV2
    //loadFLACDataChunk();
    #endif
    
      /* process MP4 data in read buffer */
    if(getH264DecodeReady()) {
      uint32_t startTime = micros();

      int w = H264_OUTPUT_W;
      int h = H264_OUTPUT_H;

      dbgPrint("Pushing screen...");

      if(oldFramePtr != NULL) {
        convertPushLines(oldFramePtr, w, h);
      }
      uint8_t* frame_buffer = NULL;

      dbgPrint("Core1 fixing and decoding...");

      int bufSize = -1;
      uint8_t* bufPtr;

      uint32_t ret_code = H264BSD_ERROR;

      if(lastH264Sample) {
        dbgPrint("Loading last h264 sample!!!");
        lastH264Sample = false;
        bufSize = getH264LastSampleSize();
        bufPtr = getH264LastSampleBuf();
        
        if(bufPtr) fixAVCCStream(bufPtr, bufSize);
        ret_code = h264bsdDecode(decHd, bufPtr, bufSize, &frame_buffer, (u32*)(&w), (u32*)(&h));
        setH264DecodeDone();
      } else {
        bufPtr = getH264SamplePtr(getH264TrafCurrentSample() - 1, &bufSize);
          if(bufPtr) fixAVCCStream(bufPtr, bufSize);
        
        ret_code = h264bsdDecode(decHd, bufPtr, bufSize, &frame_buffer, (u32*)(&w), (u32*)(&h));

        setH264DecodeDone();
      }

      

      int push_ready = false;
      strncpy(getVolumeString(), "|-------|", 10);
      getVolumeString()[1 + volumeSetting] = '+';

      if(ret_code == H264BSD_ERROR) {
        dbgPrint("ERROR: decode error\n");
      } else if(ret_code == H264BSD_PARAM_SET_ERROR) {
        dbgPrint("ERROR: Serious error in decoding, failed to activate param sets\n");
      } else if(ret_code == H264BSD_RDY) {
        dbgPrint("h264 decoder expecting more data...\n\r");
      } else if(ret_code == H264BSD_PIC_RDY) {
        push_ready = true;
          startTime = micros() - startTime;
          dbgPrint(String("SUCCESS: Picture ready: ") + String(startTime) + "\n"); // %ld ms, f=%ld\n", end - start, clock_get_hz(clk_sys));
          dbgPrint("Total dropped frames: "+String(totalDroppedFrames));
        
          uint64_t t1 = micros();
      } else if(ret_code == H264BSD_MEMALLOC_ERROR) {
        dbgPrint("ERROR: Not enough memory (core1)\n");
      } else {
        //Unhandled decoder state, don't do anything
      }

      if(push_ready) {
          if(startTime < (targetFrameTime)) {
            dbgPrint("Frame ahead!");
            oldFramePtr = frame_buffer;
          } else if(startTime < (3*targetFrameTime/2)) {
            // Push next frame right now!
            convertPushLines(frame_buffer, w, h);
            oldFramePtr = NULL;
          } else {
            dbgPrint("No time to push frame!!");
            // Skip pushing the frame altogether
            oldFramePtr = NULL;
            totalDroppedFrames++;
          }
      }
    }
  } else 
  #endif
  if(isAVIStreamAvailable()) {
    dbgPrint("Decoding AVI stream!!");
    #ifndef TinyTVKit
    oldFramePtr = NULL;
    #endif
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
  uint64_t t1 = micros();
  dbgPrint("Decode loop in "+String((int)(t1-t0))+" us");
}
