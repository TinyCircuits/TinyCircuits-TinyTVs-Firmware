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

#include "JPEGStreamer.h"

extern const void dbgPrint(const char* s);
extern const void dbgPrint(const String& s);

JPEGStreamer::JPEGStreamer(JPEGDEC *_jpeg){
  jpeg = _jpeg;
}

extern int IMG_XOFF, IMG_YOFF, IMG_W, IMG_H, HW_VIDEO_W, HW_VIDEO_H;
extern bool live;
extern TinyScreen display;

void JPEGStreamer::decode(uint8_t *jpegBuffer, const uint16_t &jpegBufferReadCount, JPEG_DRAW_CALLBACK *pfnDraw){
  // Open and decode in memory
  if (!jpeg->openRAM(jpegBuffer, jpegBufferReadCount, pfnDraw)){
    dbgPrint("Could not open frame from RAM! Error: ");
    dbgPrint(String(jpeg->getLastError()));
    dbgPrint("See https://github.com/bitbank2/JPEGDEC/blob/master/src/JPEGDEC.h#L83");
  }
  if(IMG_W != jpeg->getWidth() || IMG_H != jpeg->getHeight()){
    //display.clearScreen();
    IMG_XOFF = (HW_VIDEO_W-jpeg->getWidth())/2;
    IMG_YOFF = (HW_VIDEO_H-jpeg->getHeight())/2;
    IMG_W = jpeg->getWidth();
    IMG_H = jpeg->getHeight();
  }
  jpeg->setPixelType(RGB565_BIG_ENDIAN);
  jpeg->setMaxOutputSize(2048);
  jpeg->decode(0, 0, 0);
}
