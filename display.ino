#include <TinyScreen.h>
#include "globals_and_buffers.h"

#define byteswap(x) (x >> 8) | (x << 8)
static uint32_t seed = 0xDEADBEEF;

inline uint32_t qrand()
{
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return seed;
}

// TV effect colors
const uint16_t colorOrMask1 = 0x8410;
const uint16_t colorOrMask2 = 0xC618;
const uint16_t colorOrMask3 = 0xE71C;
const uint16_t colorOrMask4 = 0xF79E;

const uint16_t colorAndMask1 = 0x0861;
const uint16_t colorAndMask2 = 0x18E3;
const uint16_t colorAndMask3 = 0x39E7;
const uint16_t colorAndMask4 = 0x7BEF;

int decodeJPEGIfAvailable() {
  if (!getFilledJPEGBuffer() && !live)
    return 1;
  streamer.decode(getFilledJPEGBuffer(), getJPEGBufferLength(), JPEGDraw);
  JPEGBufferDecoded();
  return 0;
}

#ifdef TinyTVKit

// SAMD21 board specific functions

TinyScreen display = TinyScreen(TinyScreenPlus);
#include "splashes/FileNotFoundSplash_96x64.h"
#include "splashes/NoCardSplash_96x64.h"
#include "splashes/StorageErrorSplash_96x64.h"
#include "splashes/PlaybackErrorSplash_96x64.h"

int HW_VIDEO_W = VIDEO_W;
int HW_VIDEO_H = VIDEO_H;
int IMG_XOFF = 0;
int IMG_YOFF = 0;
int IMG_W = VIDEO_W;
int IMG_H = VIDEO_H;

#include <JPEGDEC.h>                // minor customization

#include <GraphicsBuffer2.h>

GraphicsBuffer2 screenBuffer = GraphicsBuffer2(VIDEO_W, 16, colorDepth16BPP);

// JPEG callback is a framebuffer blit
int JPEGDraw(JPEGDRAW* block) {
  if(staticTimer > 0)
  {
    const int iters = 1;
    for(int i = 0; i < iters; i++)
    {
      uint32_t staticPos = 0;
      while (staticPos < block->iWidth * block->iHeight) {
        /*
        uint8_t currentRand = rand();
        uint8_t currentRandSmall = ((currentRand >> 6 - (rand()) / 2)) & 3;
        */
        uint8_t currentRand = qrand();
        uint8_t currentRandSmall = ((currentRand >> 4) & 3);
        if (currentRandSmall == 3) {
          ((uint16_t *)block->pPixels)[staticPos] = byteswap(colorAndMask1);//black
        } else if (currentRandSmall == 2) {
          ((uint16_t *)block->pPixels)[staticPos] = byteswap(colorOrMask4);//white
        } else if (currentRandSmall == 1) {
          ((uint16_t *)block->pPixels)[staticPos] = byteswap(colorOrMask1);//black/grey
        }
        staticPos += (currentRand & 3) + 1;
      }
    }
  }
  if(nextVideoTimer != 0)
  {
    for (int i = 0; i < block->iWidth * block->iHeight; i++)
    {
      ((uint16_t *)block->pPixels)[i] &= byteswap(0b1110011110011100);
    }
  }
  screenBuffer.setBuffer((uint8_t *)block->pPixels);
  if (block->y < 8 && block->x+block->iWidth > IMG_W/2) {
    char buf[10];
    // Render stylizations
    if (showChannelNumber) {
      if ( showChannelTimer ) {
        sprintf(buf, "CH%.2i", channelNumber);
        if (IMG_H > 64) {
          screenBuffer.setBuffer((uint8_t *)frameBuf);
          screenBuffer.setCursor(IMG_W - 50, 5);
          screenBuffer.print(buf);
        } else {
          screenBuffer.setCursor(IMG_W - 25, 5);
          screenBuffer.print(buf);
        }
        showChannelTimer--;
      }
    }
  }
  if (block->y == 48) { 
    if (showVolumeTimer > 0)
    {
      char volumeString[] = "|---------|";
      volumeString[1 + (soundVolume * 8) / 255] = '+';
      if (timeStamp) {
        if (VIDEO_H > 64) {
          //renderer.drawStr(VIDEO_W - strlen(volumeString) * 7, VIDEO_H - 20, volumeString, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
        } else {
          //renderer.drawStr((VIDEO_W / 2) - 28, VIDEO_H - 18, volumeString, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
        }
      } else {
        if (VIDEO_H > 64) {
          //renderer.drawStr((VIDEO_W / 2) - 20, VIDEO_H - 25, volumeString, uraster::color(255, 255, 255), liberationSansNarrow_14ptFontInfo);
          screenBuffer.setBuffer((uint8_t *)frameBuf);
          screenBuffer.setCursor((VIDEO_W / 2) - 28, VIDEO_H - 15);
          screenBuffer.print(volumeString);
        } else {
          //renderer.drawStr((VIDEO_W / 2) - 28, VIDEO_H - 15, volumeString, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
          screenBuffer.setCursor((VIDEO_W / 2) - 28, VIDEO_H - 15 - (16 * 3));
          screenBuffer.print(volumeString);
        }
      }
      showVolumeTimer--;
    }
  }
  if ( block->x == 0 && block->y == 0) {
    display.endTransfer();
    display.setX(block->x+IMG_XOFF, block->iWidth - 1+IMG_XOFF);
    //display.setY(block->y, block->y + block->iHeight - 1);
    display.setY(block->y+IMG_YOFF, VIDEO_H - 1 + IMG_YOFF);
    display.startData();
  }
  display.writeBufferDMA((uint8_t *)block->pPixels, (block->iWidth * block->iHeight) * 2);
  if (block->y != 48) {
    delayMicroseconds(500 + 600);
  }
  return 1;
}

void JPEGBufferFilled(int length) {
  decoderDataLength[currentWriteBuf] = length;
  frameDecoded[currentWriteBuf] = false;
  frameReady[currentWriteBuf] = true;
}

uint8_t * getFilledJPEGBuffer() {
  if (frameReady[/*1 - */currentWriteBuf]) {
    currentDecodeBuf = /*1 - */currentWriteBuf;
    return videoBuf[/*1 - */currentWriteBuf];
  }
  return NULL;
}

void JPEGBufferDecoded() {
  if(staticTimer > 0) staticTimer--;
  decoderDataLength[currentDecodeBuf] = 0;
  frameReady[currentDecodeBuf] = false;
  frameDecoded[currentDecodeBuf] = true;
}

void  displayPlaybackError(char * filename) {
  dbgPrint("Playback error: " + String(filename));
  display.endTransfer();
  display.setX(VIDEO_X, VIDEO_X + VIDEO_W - 1);
  display.setY(VIDEO_Y, VIDEO_Y + VIDEO_H - 1);
  display.startData();
  display.writeBufferDMA((uint8_t *)PlaybackErrorSplash_96x64, VIDEO_W * VIDEO_H * 2);
}

void  displayCardNotFound() {
  dbgPrint("Card not found!");
  display.endTransfer();
  display.setX(VIDEO_X, VIDEO_X + VIDEO_W - 1);
  display.setY(VIDEO_Y, VIDEO_Y + VIDEO_H - 1);
  display.startData();
  display.writeBufferDMA((uint8_t *)NoCardSplash_96x64, VIDEO_W * VIDEO_H * 2);
}

void  displayFileSystemError() {
  dbgPrint("Filesystem Error!");
  display.endTransfer();
  display.setX(VIDEO_X, VIDEO_X + VIDEO_W - 1);
  display.setY(VIDEO_Y, VIDEO_Y + VIDEO_H - 1);
  display.startData();
  display.writeBufferDMA((uint8_t *)StorageErrorSplash_96x64, VIDEO_W * VIDEO_H * 2);
}

void  displayNoVideosFound() {
  dbgPrint("No Videos Found!");
  display.endTransfer();
  display.setX(VIDEO_X, VIDEO_X + VIDEO_W - 1);
  display.setY(VIDEO_Y, VIDEO_Y + VIDEO_H - 1);
  display.startData();
  display.writeBufferDMA((uint8_t *)FileNotFoundSplash_96x64, VIDEO_W * VIDEO_H * 2);
}

#else

// RP2040 board specific functions

#if defined(TinyTVMini)
TinyScreen display = TinyScreen(RP2040TVMini);
#include "splashes/FileNotFoundSplash_64x64.h"
#include "splashes/NoCardSplash_64x64.h"
#include "splashes/StorageErrorSplash_64x64.h"
#include "splashes/PlaybackErrorSplash_64x64.h"
#else
TinyScreen display = TinyScreen(RP2040TV);
#include "splashes/FileNotFoundSplash_216x135.h"
#include "splashes/NoCardSplash_216x135.h"
#include "splashes/StorageErrorSplash_216x135.h"
#include "splashes/PlaybackErrorSplash_216x135.h"
#endif

int HW_VIDEO_W = VIDEO_W;
int HW_VIDEO_H = VIDEO_H;
int IMG_XOFF = 0;
int IMG_YOFF = 0;
int IMG_W = VIDEO_W;
int IMG_H = VIDEO_H;

#include <JPEGDEC.h>                // minor customization

#include <GraphicsBuffer2.h>

GraphicsBuffer2 screenBuffer = GraphicsBuffer2(VIDEO_W, VIDEO_H, colorDepth16BPP);

// JPEG callback is a framebuffer blit
int JPEGDraw(JPEGDRAW* block) {
  if(staticTimer > 0)
  {
    #if defined(TinyTVMini)
    const int iters = 2;
    #else
    const int iters = 1;
    #endif
    for(int i = 0; i < iters; i++)
    {
      uint32_t staticPos = 0;
      while (staticPos < block->iWidth * block->iHeight) {
        uint8_t currentRand = qrand();
        uint8_t currentRandSmall = ((currentRand >> 4) & 3);
        if (currentRandSmall == 3) {
          ((uint16_t *)block->pPixels)[staticPos] = byteswap(colorAndMask1);//black
        } else if (currentRandSmall == 2) {
          ((uint16_t *)block->pPixels)[staticPos] = byteswap(colorOrMask4);//white
        } else if (currentRandSmall == 1) {
          ((uint16_t *)block->pPixels)[staticPos] = byteswap(colorOrMask1);//black/grey
        }
        staticPos += (currentRand & 3) + 1;
      }
    }
  }
  if(nextVideoTimer != 0)
  {
    for (int i = 0; i < block->iWidth * block->iHeight; i++)
    {
      ((uint16_t *)block->pPixels)[i] &= byteswap(0b1110011110011100);
    }
  }
  #if !defined(TinyTVMini)
  int fbpos = (block->y)*IMG_W+block->x;
  int bpos = 0;
  int maxWidth = min(IMG_W-block->x, block->iWidth);
  for(int by = 0; by < block->iHeight; by++)
  {
    for(int bx = 0; bx < maxWidth; bx++)
    {
      frameBuf[fbpos++] = block->pPixels[bpos++];
    }
    fbpos += IMG_W-maxWidth;
    bpos += block->iWidth-maxWidth;
  }
  #endif
  screenBuffer.setBuffer((uint8_t *)block->pPixels);
  if (block->y < 8 && block->x+block->iWidth > IMG_W/2) {
    char buf[10];
    // Render stylizations
    if (showChannelNumber) {
      if ( showChannelTimer ) {
        sprintf(buf, "CH%.2i", channelNumber);
        if (IMG_H > 64) {
          screenBuffer.setBuffer((uint8_t *)frameBuf);
          screenBuffer.setCursor(IMG_W - 50, 5);
          screenBuffer.print(buf);
        } else {
          screenBuffer.setCursor(IMG_W - 25, 5);
          screenBuffer.print(buf);
        }
        showChannelTimer--;
      }
    }
  }
  #if defined(TinyTVMini)
  if (block->y == 48) {
  #else
  if (block->y + block->iHeight > VIDEO_H-16) {
  #endif    
    if (showVolumeTimer > 0)
    {
      char volumeString[] = "|---------|";
      volumeString[1 + (soundVolume * 8) / 255] = '+';
      if (timeStamp) {
        if (VIDEO_H > 64) {
          //renderer.drawStr(VIDEO_W - strlen(volumeString) * 7, VIDEO_H - 20, volumeString, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
        } else {
          //renderer.drawStr((VIDEO_W / 2) - 28, VIDEO_H - 18, volumeString, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
        }
      } else {
        if (VIDEO_H > 64) {
          //renderer.drawStr((VIDEO_W / 2) - 20, VIDEO_H - 25, volumeString, uraster::color(255, 255, 255), liberationSansNarrow_14ptFontInfo);
          screenBuffer.setBuffer((uint8_t *)frameBuf);
          screenBuffer.setCursor((VIDEO_W / 2) - 28, VIDEO_H - 15);
          screenBuffer.print(volumeString);
        } else {
          //renderer.drawStr((VIDEO_W / 2) - 28, VIDEO_H - 15, volumeString, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
          screenBuffer.setCursor((VIDEO_W / 2) - 28, VIDEO_H - 15 - (16 * 3));
          screenBuffer.print(volumeString);
        }
      }
      showVolumeTimer--;
    }
  }
  #if defined(TinyTVMini)
  int fbpos = (block->y+IMG_YOFF)*IMG_W+block->x+IMG_XOFF;
  int bpos = 0;
  int maxWidth = min(IMG_W-block->x-IMG_XOFF, block->iWidth);
  for(int by = 0; by < block->iHeight; by++)
  {
    for(int bx = 0; bx < maxWidth; bx++)
    {
      frameBuf[fbpos++] = block->pPixels[bpos++];
    }
    fbpos += IMG_W-maxWidth;
    bpos += block->iWidth-maxWidth;
  }
  #endif
  return 1;
}

void JPEGBufferFilled(int length) {
  decoderDataLength[currentWriteBuf] = length;
  frameDecoded[currentWriteBuf] = false;
  frameReady[currentWriteBuf] = true;
  currentWriteBuf = 1 - currentWriteBuf;
}

uint8_t * getFilledJPEGBuffer() {
  if (frameReady[1 - currentWriteBuf]) {
    currentDecodeBuf = 1 - currentWriteBuf;
    return videoBuf[1 - currentWriteBuf];
  }
  return NULL;
}

void JPEGBufferDecoded() {
  while(!display.getReadyStatusDMA()){} 
  display.startCommand();
  display.setX(VIDEO_X+IMG_XOFF, VIDEO_X+IMG_XOFF+IMG_W-1);
  display.setY(VIDEO_Y+IMG_YOFF, VIDEO_Y+IMG_YOFF+IMG_H-1);
  display.endTransfer();
  display.startData();
  display.writeBufferDMA((uint8_t*)frameBuf, IMG_W*IMG_H*2);
  if(staticTimer > 0) staticTimer--;
  decoderDataLength[currentDecodeBuf] = 0;
  frameReady[currentDecodeBuf] = false;
  frameDecoded[currentDecodeBuf] = true;
}

void  displayPlaybackError(char * filename) {
  dbgPrint("Playback error: " + String(filename));
  display.endTransfer();
  display.setX(VIDEO_X, VIDEO_X + VIDEO_W - 1);
  display.setY(VIDEO_Y, VIDEO_Y + VIDEO_H - 1);
  display.startData();
#if defined(TinyTVMini)
  display.writeBufferDMA((uint8_t*)PlaybackErrorSplash_64x64, VIDEO_W * VIDEO_H * 2);
#else
  display.writeBufferDMA((uint8_t*)PlaybackErrorSplash_216x135, VIDEO_W * VIDEO_H * 2);
#endif
}

void  displayCardNotFound() {
  dbgPrint("Card not found!");
  display.endTransfer();
  display.setX(VIDEO_X, VIDEO_X + VIDEO_W - 1);
  display.setY(VIDEO_Y, VIDEO_Y + VIDEO_H - 1);
  display.startData();
#if defined(TinyTVMini)
  display.writeBufferDMA((uint8_t*)NoCardSplash_64x64, VIDEO_W * VIDEO_H * 2);
#else
  display.writeBufferDMA((uint8_t*)NoCardSplash_216x135, VIDEO_W * VIDEO_H * 2);
#endif
}

void  displayFileSystemError() {
  dbgPrint("Filesystem Error!");
  display.endTransfer();
  display.setX(VIDEO_X, VIDEO_X + VIDEO_W - 1);
  display.setY(VIDEO_Y, VIDEO_Y + VIDEO_H - 1);
  display.startData();
#if defined(TinyTVMini)
  display.writeBufferDMA((uint8_t*)StorageErrorSplash_64x64, VIDEO_W * VIDEO_H * 2);
#else
  display.writeBufferDMA((uint8_t*)StorageErrorSplash_216x135, VIDEO_W * VIDEO_H * 2);
#endif
}

void  displayNoVideosFound() {
  dbgPrint("No Videos Found!");
  display.endTransfer();
  display.setX(VIDEO_X, VIDEO_X + VIDEO_W - 1);
  display.setY(VIDEO_Y, VIDEO_Y + VIDEO_H - 1);
  display.startData();
#if defined(TinyTVMini)
  display.writeBufferDMA((uint8_t*)FileNotFoundSplash_64x64, VIDEO_W * VIDEO_H * 2);
#else
  display.writeBufferDMA((uint8_t*)FileNotFoundSplash_216x135, VIDEO_W * VIDEO_H * 2);
#endif
}

void displayUSBMSCmessage() {
  #ifdef TinyTVMini
  display.setCursor(5, 10);
  display.print("USB Mode");
  display.setCursor(5, 20);
  display.print("Eject or");
  display.setCursor(5, 30);
  display.print("disconnect");
  display.setCursor(5, 40);
  display.print("to continue");
  #else
  display.setCursor(85+24, 45);
  display.print("USB Mode");
  display.setCursor(85+24, 55);
  display.print("Eject or");
  display.setCursor(85+24, 65);
  display.print("disconnect");
  display.setCursor(85+24, 75);
  display.print("to continue");
  #endif
  writeToScreenDMA(frameBuf, VIDEO_W * VIDEO_H);
}

#endif

// These functions are similar across all platforms

void initializeDisplay() {
  // Initialize TFT
  display.begin();
  display.setBitDepth(1);
  display.setColorMode(TSColorModeRGB);
  #ifndef TinyTVKit
  display.setFlip(false);
  #else
  display.setFlip(true);
  #endif
  display.clearScreen();
  display.setFont(thinPixel7_10ptFontInfo);
  display.initDMA();


  if (screenBuffer.begin()) {
    dbgPrint("malloc error");
  }
  screenBuffer.setFont(thinPixel7_10ptFontInfo);
#ifndef TinyTVKit
#ifdef TinyTVMini
  digitalWrite(9, HIGH);
#else
  digitalWrite(9, LOW);
#endif
#endif
}

void resetBuffers() {
  frameReady[0] = false; frameReady[1] = false;
  frameDecoded[0] = true; frameDecoded[1] = true;
  dbgPrint("resetBuffers");
}

void setScreenAddressWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  display.endTransfer();
  display.setX(x, x+w);
  display.setY(y, y+h);
  display.startData();
}

int getJPEGBufferLength() {
  return decoderDataLength[currentDecodeBuf];
}

uint8_t * getFreeJPEGBuffer() {
  if (millis() - lastBufferAssignment > 250) {
    resetBuffers();
  }
  if (frameDecoded[currentWriteBuf] == true && frameReady[currentWriteBuf] == false) {
    lastBufferAssignment = millis();
    return videoBuf[currentWriteBuf];
  }
  return NULL;
}

void core2Loop()
{
  if (TVscreenOffMode) {
    return;
  } 
  if (decodeJPEGIfAvailable()) {
    return;
  }

  return;
}

void writeToScreenDMA(uint16_t * bufToWrite, uint16_t count) {
  display.writeBufferDMA((uint8_t *)bufToWrite, count * 2);
}

void waitForScreenDMA() {
  while(!display.getReadyStatusDMA()) {}  
}

void writeScreenBuffer() {
  setScreenAddressWindow(VIDEO_X, VIDEO_Y, VIDEO_W, VIDEO_H);
  writeToScreenDMA(frameBuf, VIDEO_W * VIDEO_H);
  waitForScreenDMA();
}

void clearDisplay() {
  waitForScreenDMA();
  writeToScreenDMA(frameBuf, VIDEO_W * VIDEO_H);
}

void changeChannelEffect()
{
  waitForScreenDMA();
  staticTimer = (1000000/targetFrameTime) / 3;
}

void tubeOffEffect() {
  delay(5);
  if (pauseRadius) {
    int xCircle = pauseRadius / 2;
    int yCircle = 0;
    int radiusError = 1 - xCircle;
    int radiusLimits[VIDEO_H];
    uint16_t staticBuf[VIDEO_W];
    memset(radiusLimits, 0, sizeof(radiusLimits));
    while (xCircle >= yCircle) {
      radiusLimits[64 + yCircle] = xCircle * 3 / 2;
      radiusLimits[64 - yCircle] = xCircle * 3 / 2;
      radiusLimits[64 - xCircle] = yCircle * 3 / 2;
      radiusLimits[64 + xCircle] = yCircle * 3 / 2;
      yCircle++;
      if (radiusError < 0)
      {
        radiusError += 2 * yCircle + 1;
      } else {
        xCircle--;
        radiusError += 2 * (yCircle - xCircle) + 1;
      }
    }
    // Need a full framebuffer for this to work
    for (int y = 0; y < VIDEO_H; y++) {
      waitForScreenDMA();
      memset(staticBuf, 0, VIDEO_W * sizeof(uint16_t));
      for (int x = 0; x < VIDEO_W / 2 - radiusLimits[y]; x++) {
        ((uint16_t *)staticBuf)[x] &= byteswap(0x39E7);
        ((uint16_t *)staticBuf)[x] = (((uint16_t *)staticBuf)[x] >> 1);
      }
      for (int x = VIDEO_W / 2 + radiusLimits[y]; x < VIDEO_W; x++) {
        ((uint16_t *)staticBuf)[x] &= byteswap(0x39E7);
        ((uint16_t *)staticBuf)[x] = (((uint16_t *)staticBuf)[x] >> 1);
      }
      setScreenAddressWindow(VIDEO_X, y+1,VIDEO_W, 1);
      writeToScreenDMA(staticBuf, VIDEO_W);
    }
    memset(staticBuf, 0, VIDEO_W * sizeof(uint16_t));
    for (int y = 0; y < VIDEO_H; y++) {
      waitForScreenDMA();
      memset(staticBuf, 0, VIDEO_W * sizeof(uint16_t));
      if (radiusLimits[y]) {
        for (int x = VIDEO_W / 2 - radiusLimits[y] - 3; x < VIDEO_W / 2 + radiusLimits[y] + 3; x) {
          uint8_t currentRand = qrand();
          uint8_t currentRandSmall = ((currentRand >> 4) & 3);
          if (x == VIDEO_W / 2 - radiusLimits[y] - 3)
            x += (currentRand & 3);
          if (currentRandSmall == 3) {
            ((uint16_t *)staticBuf)[x] = byteswap(colorAndMask1);//black
          } else if (currentRandSmall == 2) {
            ((uint16_t *)staticBuf)[x] = byteswap(colorOrMask4);//white
          } else if (currentRandSmall == 1) {
            ((uint16_t *)staticBuf)[x] = byteswap(colorOrMask1);//black/grey
          }
          x += (currentRand & 3) * 2;
        }
      }
      setScreenAddressWindow(VIDEO_X, y+1,VIDEO_W, 1);
      writeToScreenDMA(staticBuf, VIDEO_W);
    }
    pauseRadius -= pauseRadius / 8;
    if (pauseRadius < 12) {
      pauseRadius--;

      if (pauseRadius < 6 && pauseRadius > 3 ) {
        for (int y = VIDEO_H / 2 - 2; y < VIDEO_H / 2 + 1; y++) {
          waitForScreenDMA();
          memset(staticBuf, 0, VIDEO_W * sizeof(uint16_t));
          for (int x = VIDEO_W / 2 - 30; x < VIDEO_W / 2 + 30; x++) {
            uint8_t currentRand = rand();
            uint8_t currentRandSmall = ((currentRand >> 4) & 3);
            if (x == VIDEO_W / 2 - radiusLimits[y] - 3)
              x += (currentRand & 3);
            if (currentRandSmall == 3) {
            ((uint16_t *)staticBuf)[x] = byteswap(colorAndMask1);//black
          } else if (currentRandSmall == 2) {
            ((uint16_t *)staticBuf)[x] = byteswap(colorOrMask4);//white
          } else if (currentRandSmall == 1) {
            ((uint16_t *)staticBuf)[x] = byteswap(colorOrMask1);//black/grey
          }
            x += (currentRand & 3) * 2;
          }
          setScreenAddressWindow(VIDEO_X, y,VIDEO_W, 1);
          writeToScreenDMA(staticBuf, VIDEO_W);
        }
      }
      if (pauseRadius < 6) {
        delay((6 - pauseRadius) * 15);
      }
    }
    if (pauseRadius <= 0) {
      pauseRadius = 0;
    }
  }
  setScreenAddressWindow(VIDEO_X, VIDEO_Y, VIDEO_W, VIDEO_H);
}
