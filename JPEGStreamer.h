//-------------------------------------------------------------------------------
//  TinyCircuits RP2040TV Video Player, JPEG Streaming and Decode Component
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

#ifndef JPEG_STREAMER_H
#define JPEG_STREAMER_H

#include <JPEGDEC.h>
#include <stdlib.h>
#include <TinyScreen.h>

class JPEGStreamer{
  public:
    JPEGStreamer(JPEGDEC *_jpeg);

    //void decode(uint8_t *jpegBuffer0, uint8_t *jpegBuffer1, uint16_t *screenBuffer, JPEG_DRAW_CALLBACK *pfnDraw);  // Pass JPEGDec callback function (No longer necessary, one JPEG buffer)
    void decode(uint8_t *jpegBuffer, const uint16_t &jpegBufferReadCount, JPEG_DRAW_CALLBACK *pfnDraw);   // Pass JPEGDec callback function

  private:

    JPEGDEC *jpeg;
};

#endif