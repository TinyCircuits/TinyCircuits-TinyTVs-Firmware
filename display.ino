//-------------------------------------------------------------------------------
//  TinyCircuits TinyTV Firmware
//
//  Changelog:
//  05/26/2023 Initial Release for TinyTV 2/Mini
//  02/08/2023 Cross-platform base committed
//
//  Written by Mason Watmough, Ben Rose, and Jason Marcum for TinyCircuits, http://TinyCircuits.com
//
//-------------------------------------------------------------------------------


#include "src/GraphicsBuffer2/GraphicsBuffer2.h"

#if DOUBLE_BUFFER
uint16_t frameBuf[VIDEO_W * VIDEO_H];
GraphicsBuffer2 screenBuffer = GraphicsBuffer2(VIDEO_W, VIDEO_H, colorDepth16BPP);
#else
uint16_t * frameBuf; // should not be needed, avoid compile errors
GraphicsBuffer2 screenBuffer = GraphicsBuffer2(VIDEO_W, 16, colorDepth16BPP);
#endif



int HW_VIDEO_W = VIDEO_W;
int HW_VIDEO_H = VIDEO_H;
int IMG_XOFF = 0;
int IMG_YOFF = 0;
int IMG_W = VIDEO_W;
int IMG_H = VIDEO_H;



int JPEGDraw(JPEGDRAW* block) {
  //  if (block->y == 0) {
  //    cdc.print(block->iWidth);
  //    cdc.println(block->iHeight);
  //  }
  screenBuffer.setWidth(block->iWidth);
  screenBuffer.setBuffer((uint8_t *)block->pPixels);

  drawStatic((uint16_t *)block->pPixels, block->iWidth, block->iHeight);
  drawVolume(block);
  drawChannelNumber(block);
  drawCorners(block);




  // if DOUBLE_BUFFER is true, first copy block out to static buffer for drawing text
#if DOUBLE_BUFFER
  //int maxWidth = min(IMG_W - block->x - IMG_XOFF, block->iWidth);
  int maxWidth = block->iWidth;
  //cdc.print(maxWidth);
  //cdc.print(" ");
  if (block->x + IMG_XOFF + block->iWidth >= VIDEO_W) {
    maxWidth = VIDEO_W - (block->x + IMG_XOFF );
  }
  //cdc.print(maxWidth);
  //cdc.print(" ");
  //cdc.print(block->y);
  //cdc.print(" ");
  //cdc.println(block->iHeight);
  //cdc.print(" ");
  //cdc.print(IMG_YOFF);

  for (int by = 0; by < block->iHeight; by++) {
    int fbpos = (block->y + IMG_YOFF + by) * VIDEO_W + block->x + IMG_XOFF;

    //cdc.print(" ");
    //cdc.print(fbpos);
    int bpos = (by * block->iWidth) + 0/*block->x*/ /*+ IMG_XOFF*/;
    //cdc.print(" ");
    //cdc.println(bpos);
    for (int bx = 0; bx < maxWidth; bx++) {
      frameBuf[fbpos++] = block->pPixels[bpos++];
    }
    //fbpos += IMG_W - maxWidth;
    //bpos += block->iWidth - maxWidth;
  }
  screenBuffer.setBuffer((uint8_t *)frameBuf);
#else
  screenBuffer.setBuffer((uint8_t *)block->pPixels);
#endif

  if (DOUBLE_BUFFER == false) {
    while (!display.getReadyStatusDMA()) {}
    display.endTransfer();
    display.setX(block->x + IMG_XOFF, block->iWidth - 1 + IMG_XOFF);
    //display.setY(block->y, block->y + block->iHeight - 1);
    display.setY(block->y + IMG_YOFF, VIDEO_H - 1 + IMG_YOFF);
    display.startData();
    display.writeBufferDMA((uint8_t *)block->pPixels, (block->iWidth * block->iHeight) * 2);
    if (block->y != 48) {
      delayMicroseconds(500 + 600);
    }
  } else {
    //cdc.print(block->y + block->iHeight);
    //cdc.print(" ");
    //cdc.println(block->x + block->iWidth);
    //if (block->y + block->iHeight >= (VIDEO_H - 2) && block->x + block->iWidth >= (VIDEO_W - 2)) { //assume last block
    if (block->y + block->iHeight >= (IMG_H - 2) && block->x + block->iWidth >= (IMG_W - 2)) { //assume last block
      //cdc.println("dma");
      while (!display.getReadyStatusDMA()) {}
      setScreenAddressWindow(VIDEO_X, VIDEO_Y, VIDEO_W, VIDEO_H);
      display.writeBufferDMA((uint8_t*)frameBuf, VIDEO_W * VIDEO_H * 2);
      //cdc.println("DMA started");
    }
  }
  return 1;
}


void newJPEGFrameSize(int newWidth, int newHeight) {
  if ( newWidth <= VIDEO_W && newHeight <= VIDEO_H) {
    IMG_XOFF = (VIDEO_W - newWidth) / 2;
    IMG_YOFF = (VIDEO_H - newHeight) / 2;
    IMG_W = newWidth;
    IMG_H = newHeight;
  } else {
    IMG_XOFF = 0;
    IMG_YOFF = 0;
    //IMG_W = VIDEO_W;
    //IMG_H = VIDEO_H;
    IMG_W = newWidth;
    IMG_H = newHeight;

  }
}


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

  //display.drawRect(30,0,240-30,135,1,0xFFFF);
  //  while(1);



#ifdef TinyTVKit
  if (screenBuffer.begin()) {
    dbgPrint("malloc error");
  }
  screenBuffer.setFont(thinPixel7_10ptFontInfo);
#endif

#ifndef TinyTVKit

#ifdef TinyTVMini
  digitalWrite(9, HIGH); //needed?
  screenBuffer.setFont(thinPixel7_10ptFontInfo);
  screenBuffer.fontColor(0xFFFF, ALPHA_COLOR);
#else
  digitalWrite(9, LOW); //needed?
  screenBuffer.setFont(liberationSansNarrow_14ptFontInfo);
  screenBuffer.fontColor(0xFFFF, ALPHA_COLOR);
#endif
#endif

  setCornerRadius(26);
}

void displayOff() {
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

}

void displayOn() {
#ifndef TinyTVKit
#ifdef TinyTVMini
  digitalWrite(9, HIGH);
#else
  digitalWrite(9, LOW);
#endif
#endif
  display.on();
}

void setScreenAddressWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  display.endTransfer();
  display.setX(x, x + w);
  display.setY(y, y + h);
  display.startData();
}

void writeToScreenDMA(uint16_t * bufToWrite, uint16_t count) {
  display.writeBufferDMA((uint8_t *)bufToWrite, count * 2);
}

void waitForScreenDMA() {
  while (!display.getReadyStatusDMA()) {}
}

void writeScreenBuffer() {
  setScreenAddressWindow(VIDEO_X, VIDEO_Y, VIDEO_W, VIDEO_H);
  writeToScreenDMA(frameBuf, VIDEO_W * VIDEO_H);
  waitForScreenDMA();
}

void clearDisplay() {
  waitForScreenDMA();
  display.clearScreen();
  //writeToScreenDMA(frameBuf, VIDEO_W * VIDEO_H);
}

void  displayPlaybackError(char * filename) {
  dbgPrint("Playback error: " + String(filename));
  waitForScreenDMA();
  setScreenAddressWindow(VIDEO_X, VIDEO_Y, VIDEO_W, VIDEO_H);
  writeToScreenDMA((uint16_t *)PLAYBACK_ERROR_SPLASH, VIDEO_W * VIDEO_H);
}

void  displayCardNotFound() {
  dbgPrint("Card not found!");
  waitForScreenDMA();
  display.endTransfer();
  display.setX(VIDEO_X, VIDEO_X + VIDEO_W - 1);
  display.setY(VIDEO_Y, VIDEO_Y + VIDEO_H - 1);
  display.startData();
  display.writeBufferDMA((uint8_t *)NO_CARD_ERROR_SPLASH, VIDEO_W * VIDEO_H * 2);
}

void  displayFileSystemError() {
  dbgPrint("Filesystem Error!");
  waitForScreenDMA();
  display.endTransfer();
  display.setX(VIDEO_X, VIDEO_X + VIDEO_W - 1);
  display.setY(VIDEO_Y, VIDEO_Y + VIDEO_H - 1);
  display.startData();
  display.writeBufferDMA((uint8_t *)STORAGE_ERROR_SPLASH, VIDEO_W * VIDEO_H * 2);
}

void  displayNoVideosFound() {
  dbgPrint("No Videos Found!");
  waitForScreenDMA();
  display.endTransfer();
  display.setX(VIDEO_X, VIDEO_X + VIDEO_W - 1);
  display.setY(VIDEO_Y, VIDEO_Y + VIDEO_H - 1);
  display.startData();
  display.writeBufferDMA((uint8_t *)FILE_NOT_FOUND_SPLASH, VIDEO_W * VIDEO_H * 2);
}

#ifdef has_USB_MSC
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
  writeToScreenDMA(frameBuf, VIDEO_W * VIDEO_H);
#else
  //  display.setCursor(85 + 24, 45);
  //  display.print("USB Mode");
  //  display.setCursor(85 + 24, 55);
  //  display.print("Eject or");
  //  display.setCursor(85 + 24, 65);
  //  display.print("disconnect");
  //  display.setCursor(85 + 24, 75);
  //  display.print("to continue");
  waitForScreenDMA();
  display.endTransfer();
  display.setX(VIDEO_X, VIDEO_X + VIDEO_W - 1);
  display.setY(VIDEO_Y, VIDEO_Y + VIDEO_H - 1);
  display.startData();
  display.writeBufferDMA((uint8_t *)MASS_STORAGE_SPLASH, VIDEO_W * VIDEO_H * 2);
#endif
}
#endif
