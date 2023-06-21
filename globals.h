//-------------------------------------------------------------------------------
//  TinyCircuits RP2040TV Video Player, Hardware Initialization Component
//
//  Changelog:
//  08/12/2022 Handed off the keys to the kingdom
//  
//  02/08/2023 Cross-platform base committed
//
//  Written by Mason Watmough for TinyCircuits, http://TinyCircuits.com
//  Refactored by Jason Marcum for portability
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



typedef struct inputFlagStruct{
  bool channelUp = false;
  bool channelDown = false;
  bool volUp = false;
  bool volDown = false;
  bool mute = false;
  bool power = false;
  bool channelSet = false;
  bool volumeSet = false;
  bool settingsChanged = false;
}inputFlagStruct;


// PLAYBACK PARAMETERS
uint64_t targetFrameTime;


int volumeSetting = 3;

inputFlagStruct inputFlags;
