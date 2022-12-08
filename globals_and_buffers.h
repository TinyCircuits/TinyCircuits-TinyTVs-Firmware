//-------------------------------------------------------------------------------
//  TinyCircuits RP2040TV Video Player, Globals Component
//
//  Changelog:
//  08/12/2022 Handed off the keys to the kingdom
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

#include "screenEffects.h"

#ifdef DEBUGAPP
const void dbgPrint(const char* s) {
  Serial.println(s);
}
const void dbgPrint(const String& s) {
  Serial.println(s);
}
#else
const void dbgPrint(const char* s) {
  (void) s;
}
const void dbgPrint(const String& s) {
  (void) s;
}
#endif


SdFat32 sd;
File32 infile;
File32 dir;
ScreenEffects effects(216, 135);


// PLAYBACK PARAMETERS
uint64_t TVscreenOffModeStartTime = 0;
uint64_t targetFrameTime;



bool decodingFrame = false;
bool frameReady = false;
bool paused = false;

// SETTINGS DEFAULTS
uint8_t autoplayMode = 1;
bool timeStamp = false;
bool doStaticEffects = true;
bool showChannelNumber = true;
bool alphabetizedPlaylist = true;

bool TVscreenOffMode = false;
bool channelUpInput = false;
bool channelDownInput = false;
bool volUpInput = false;
bool volDownInput = false;
bool muteInput = false;
bool powerInput = false;
bool settingsNeedSaved = false;

int channelNumber = 0; //expects a 'nextVideo' call at startup
int showChannelTimer = 0;
uint64_t tsMillisInitial = 0;
int showVolumeTimer = 0;
uint64_t framerateHelper = 0;

int decoderDataLength = 0;
uint8_t currentWriteBuf = 0;
const int VIDEOBUF_SIZE = 1024 * 12;
uint8_t videoBuf[2][VIDEOBUF_SIZE];

// Audio buffer and size
const uint32_t AUDIOBUF_SIZE = 1024 * 8;
uint8_t audioBuf[AUDIOBUF_SIZE] = {127};

volatile int sampleIndex = 0;
volatile int loadedSampleIndex = 0;




// For tube off effect
int pauseRadius = 0;


// Sample playback stuff. All of this should be volatile because we do scary stuff in ISR

volatile int soundVolume = 64;
volatile int samplesPerFrame = 417;
volatile bool playWhiteNoise = false;
volatile bool videoHasAudio = false;
volatile bool seekLivePos = false;
