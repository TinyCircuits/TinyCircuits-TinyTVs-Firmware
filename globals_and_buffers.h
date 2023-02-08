//-------------------------------------------------------------------------------
//  TinyCircuits RP2040TV Video Player, Globals Component
//
//  Changelog:
//  08/12/2022 Handed off the keys to the kingdom
//  
//  02/08/2023 Cross-platform base committed
//
//  Written by Mason Watmough for TinyCircuits, http://TinyCircuits.com
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


#ifndef GLOBALS_INCLUDED
#define GLOBALS_INCLUDED

#include <SdFat.h>                  // Custom SdFat for RP2040
SdFat32 sd;
File32 infile;
File32 dir;
JPEGDEC jpeg;

#ifndef TinyTVKit
#include <Adafruit_TinyUSB.h>
Adafruit_USBD_MSC usb_msc;
Adafruit_USBD_CDC cdc;
#endif

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

#ifdef TinyTVKit
const int VIDEO_X = 0;
const int VIDEO_Y = 0;
const int VIDEO_W = 96;
const int VIDEO_H = 64;
const int BUF_H = 1;
uint16_t frameBuf[VIDEO_W];
#elif defined(TinyTVMini)
const int VIDEO_X = 0;
const int VIDEO_Y = 0;
const int VIDEO_W = 64;
const int VIDEO_H = 64;
const int BUF_H = 64;
uint16_t frameBuf[VIDEO_W*VIDEO_H];
#else
const int VIDEO_X = 24;
const int VIDEO_Y = 0;
const int VIDEO_W = 216;
const int VIDEO_H = 135;
const int BUF_H = 135;
uint16_t frameBuf[VIDEO_W*VIDEO_H];
#endif

#include "JPEGStreamer.h"
JPEGStreamer streamer(&jpeg);

#ifdef TinyTVKit
const int VIDEOBUF_SIZE = 1024 * 3;
const int VIDEOBUF_CNT = 1;
#else
const int VIDEOBUF_SIZE = 1024 * 20;
const int VIDEOBUF_CNT = 2;
#endif
uint8_t videoBuf[VIDEOBUF_CNT][VIDEOBUF_SIZE];

volatile bool frameReady[2] = {false, false};
volatile bool frameDecoded[2] = {true, true};
volatile int decoderDataLength[2] = {0, 0};
volatile uint8_t currentWriteBuf = 0;
volatile uint8_t currentDecodeBuf = 0;
volatile uint32_t lastBufferAssignment = 0;

// PLAYBACK PARAMETERS
uint64_t targetFrameTime;
volatile int32_t staticTimer = -1;
uint64_t powerDownTimer = 0;
volatile int32_t nextVideoTimer = 0;

bool paused = false;

// SETTINGS DEFAULTS
uint8_t autoplayMode = 1;
bool timeStamp = false;
bool doStaticEffects = true;
bool showChannelNumber = true;
bool alphabetizedPlaylist = true;

// Input Flags
bool TVscreenOffMode = false;
bool channelUpInput = false;
bool channelDownInput = false;
bool volUpInput = false;
bool volDownInput = false;
bool muteInput = false;
bool powerInput = false;
bool settingsNeedSaved = false;
bool reloadVideoList = false;

bool live = false;

int channelNumber = 0; //expects a 'nextVideo' call at startup
int showChannelTimer = 0;
uint64_t tsMillisInitial = 0;
int showVolumeTimer = 0;
uint64_t framerateHelper = 0;

// For tube off effect
int pauseRadius = 0;


// Sample playback stuff. All of this should be volatile because we do scary stuff in ISR

volatile int soundVolume = 64;
volatile bool playWhiteNoise = false;
volatile bool seekLivePos = false;

#endif
