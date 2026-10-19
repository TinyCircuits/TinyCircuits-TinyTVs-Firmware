//-------------------------------------------------------------------------------
//  TinyCircuits TinyTV Firmware
//
//  Changelog:
//  10/19/2026 MP4 playback bugfixes, performance improvements, settings-saving behavior changed
//  03/25/2026 MP4 playback update
//  05/26/2023 Initial Release for TinyTV 2/Mini
//  02/08/2023 Cross-platform base committed
//
//  Written by Mason Watmough, Ben Rose, and Jason Marcum for TinyCircuits, http://TinyCircuits.com
//
//-------------------------------------------------------------------------------


#include "src/GraphicsBuffer2/GraphicsBuffer2.h"

#if DOUBLE_BUFFER
uint16_t * frameBuf;
GraphicsBuffer2 screenBuffer = GraphicsBuffer2(VIDEO_W, VIDEO_H, colorDepth16BPP);
#else
uint16_t * frameBuf; // should not be needed, avoid compile errors
GraphicsBuffer2 screenBuffer = GraphicsBuffer2(VIDEO_W, 16, colorDepth16BPP);
#endif

uint16_t* getFrameBuffer() {
  return frameBuf;
}

void setAVIScreenBuffer() {
  if(frameBuf == NULL && DOUBLE_BUFFER) {
    frameBuf = (uint16_t*)malloc(VIDEO_W * VIDEO_H*2);
    screenBuffer.setBuffer((uint8_t *)frameBuf);
    resetBuffers();
  }
}

void setMP4ScreenBuffer() {
  if(frameBuf != NULL && DOUBLE_BUFFER) {
    free(frameBuf);
    frameBuf = NULL;
    resetBuffers();
    //display.clearScreen();
  }
}

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
  drawCornersPartial(block);




  // if DOUBLE_BUFFER is true, first copy block out to static buffer for drawing text
if (DOUBLE_BUFFER == true && block->iWidth < VIDEO_W) {
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
} else {
  screenBuffer.setBuffer((uint8_t *)block->pPixels);
}

  if (DOUBLE_BUFFER == false || block->iWidth >= VIDEO_W) {
    while (!display.getReadyStatusDMA()) {}
    display.endTransfer();
    display.setX(block->x + IMG_XOFF + VIDEO_X, block->iWidth - 1 + IMG_XOFF + VIDEO_X);
    //display.setY(block->y, block->y + block->iHeight - 1);
    display.setY(block->y + IMG_YOFF, VIDEO_H - 1 + IMG_YOFF);
    display.startData();
    display.writeBufferDMA((uint8_t *)block->pPixels, (block->iWidth * block->iHeight) * 2);
    #ifdef TinyTVKit
    if (block->y != 48) {
      delayMicroseconds(500 + 600);
    }
    #endif
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

//http://www.martinreddy.net/gfx/faqs/colorconv.faq
//https://fourcc.org/fccyvrgb.php#mikes_answer
//https://en.wikipedia.org/wiki/Rec._709
//https://en.wikipedia.org/wiki/YCbCr

static void torgb(uint16_t *sadfw, int iY1, int iY2, int iCb, int iCr) {

  uint32_t ulPixel1, ulPixel2;

  int cr = iCr;
  int cb = iCb;

  int Y1 = 4096*iY1;
  int Y2 = 4096*iY2;

  const int crrml = 6450 * (cr - 128);
  const int cbgml = - 767 * (cb - 128);
  const int crgml = - 1917 * (cr - 128);
  const int cbbml = 7601 * (cb - 128);

  int r = (Y1 + crrml) >> 4;
  int g = (Y1 + cbgml + crgml) >> 9;
  int b = (Y1 + cbbml) >> 15;

  if (r < 0) r = 0; else if (r > (255 << 8)) r = 255 << 8;
  if (g < 0) g = 0; else if (g > (255 << 3)) g = 255 << 3;
  if (b < 0) b = 0; else if (b > (255 >> 3)) b = 255 >> 3;

  ulPixel1 = (r & 0xf800) | (g & 0x07e0) | (b & 0x001f);
  ulPixel1 = ulPixel1 & 0xffff;
  ulPixel1 = ((uint16_t)ulPixel1 >> 8) | ((uint16_t)ulPixel1 << 8);

  r = (Y2 + crrml) >> 4;
  g = (Y2 + cbgml + crgml) >> 9;
  b = (Y2 + cbbml) >> 15;

  if (r < 0) r = 0; else if (r > (255 << 8)) r = 255 << 8;
  if (g < 0) g = 0; else if (g > (255 << 3)) g = 255 << 3;
  if (b < 0) b = 0; else if (b > (255 >> 3)) b = 255 >> 3;

  ulPixel2 = (r & 0xf800) | (g & 0x07e0) | (b & 0x001f);
  ulPixel2 = ulPixel2 & 0xffff;
  ulPixel2 = ((uint16_t)ulPixel2 >> 8) | ((uint16_t)ulPixel2 << 8);
  
  sadfw[0] = ulPixel1;
  sadfw[1] = ulPixel2;
}

#ifndef TinyTVKit

void convertPushLines(uint8_t* framePtr, int w, int h) {
  uint32_t Cr, Cb;
  int32_t Y1, Y2, Y3, Y4;
  int uDataOffset = w * h;
  int vDataOffset = w * h * 5 / 4;

  // display.clearWindow(0, 0, w, (VIDEO_H-h)/2);
  // display.clearWindow(0, VIDEO_Y + h, w, (VIDEO_H-h)/2);

  #ifdef TINYTV2_COMPILE

  memset(singleLineBuf[0], 0, sizeof(singleLineBuf[0]));

  setScreenAddressWindow(VIDEO_X, 0, VIDEO_W, (VIDEO_H-h)/2);
  for(int y = 0; y <= (VIDEO_H-h)/2; y++) {
    while (!display.getReadyStatusDMA()) {}
    display.writeBufferDMA((uint8_t*)singleLineBuf[0], w * 2);
  }

  setScreenAddressWindow(VIDEO_X, (VIDEO_H-h)/2+h, VIDEO_W, (VIDEO_H-h)/2+1);
  for(int y = 0; y <= (VIDEO_H-h)/2+1; y++) {
    while (!display.getReadyStatusDMA()) {}
    display.writeBufferDMA((uint8_t*)singleLineBuf[0], w * 2);
  }

  setScreenAddressWindow(VIDEO_X+w, 0, VIDEO_W-w, VIDEO_H);
  for(int y = 0; y < 1; y++) {
    while (!display.getReadyStatusDMA()) {}
    display.writeBufferDMA((uint8_t*)singleLineBuf[0], VIDEO_H * 2 * 2);
  }

  #endif

  #ifdef TINYTV2_COMPILE
  setScreenAddressWindow(VIDEO_X, VIDEO_Y + (VIDEO_H-h)/2, w-1, h + (VIDEO_H-h)/2);
  #elif defined(TINYTV2_MINI_COMPILE)
  setScreenAddressWindow(VIDEO_X, VIDEO_Y, w, h);
  #endif

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x += 2) {
      int offset = y * w ;
      Y1 = framePtr[offset + x + 0];
      Y2 = framePtr[offset + x + 1];
      int uvoffset = y/2 * w/2;
      
      Cr = framePtr[uDataOffset + uvoffset + x/2];
      Cb = framePtr[vDataOffset + uvoffset + x/2];
      torgb(singleLineBuf[lineBufIdx] + x, Y1, Y2, Cr, Cb);
    }
    while (!display.getReadyStatusDMA()) {}
    lineBufIdx = 1 - lineBufIdx;
    drawStatic(singleLineBuf[1-lineBufIdx], w, 1);
    drawChannelNumberLineBuffer(singleLineBuf[1-lineBufIdx]+3, y-4 + (VIDEO_H-h)/2);
    drawVolumeLineBuffer(singleLineBuf[1-lineBufIdx], y-8 + (VIDEO_H-h)/2);
    drawCornersPartialLineBuf(singleLineBuf[1-lineBufIdx], y + (VIDEO_H-h)/2);
    display.writeBufferDMA((uint8_t*)singleLineBuf[1-lineBufIdx], w * 2);
  }
}

#endif

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
  setCornerRadius(15);
#else
  digitalWrite(9, LOW); //needed?
  screenBuffer.setFont(liberationSansNarrow_14ptFontInfo);
  screenBuffer.fontColor(0xFFFF, ALPHA_COLOR);
  setCornerRadius(26);
#endif
#endif

}

void displayOff() {
  display.off();
}

void displayOn() {
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
  setScreenAddressWindow(VIDEO_X, VIDEO_Y, VIDEO_W - 1, VIDEO_H - 1);
//#if DOUBLE_BUFFER
//  memcpy((uint8_t*)frameBuf, (uint8_t*)PLAYBACK_ERROR_SPLASH, VIDEO_W * VIDEO_H * 2);
//  newJPEGFrameSize(VIDEO_W, VIDEO_H);
//  screenBuffer.setWidth(VIDEO_W);
//  screenBuffer.setBuffer((uint8_t *)frameBuf);
//  drawCornersFull();
//  display.writeBufferDMA((uint8_t *)frameBuf, VIDEO_W * VIDEO_H * 2);
//#else
  display.writeBufferDMA((uint8_t *)PLAYBACK_ERROR_SPLASH, VIDEO_W * VIDEO_H * 2);
//#endif
}

void  displayCardNotFound() {
  dbgPrint("Card not found!");
  waitForScreenDMA();
  setScreenAddressWindow(VIDEO_X, VIDEO_Y, VIDEO_W - 1, VIDEO_H - 1);
//#if DOUBLE_BUFFER
//  memcpy((uint8_t*)frameBuf, (uint8_t*)NO_CARD_ERROR_SPLASH, VIDEO_W * VIDEO_H * 2);
//  newJPEGFrameSize(VIDEO_W, VIDEO_H);
//  screenBuffer.setWidth(VIDEO_W);
//  screenBuffer.setBuffer((uint8_t *)frameBuf);
//  drawCornersFull();
//  display.writeBufferDMA((uint8_t *)frameBuf, VIDEO_W * VIDEO_H * 2);
//#else
  display.writeBufferDMA((uint8_t *)NO_CARD_ERROR_SPLASH, VIDEO_W * VIDEO_H * 2);
//#endif
}

void  displayFileSystemError() {
  dbgPrint("Filesystem Error!");
  waitForScreenDMA();
  setScreenAddressWindow(VIDEO_X, VIDEO_Y, VIDEO_W - 1, VIDEO_H - 1);
//#if DOUBLE_BUFFER
//  memcpy((uint8_t*)frameBuf, (uint8_t*)STORAGE_ERROR_SPLASH, VIDEO_W * VIDEO_H * 2);
//  newJPEGFrameSize(VIDEO_W, VIDEO_H);
//  screenBuffer.setWidth(VIDEO_W);
//  screenBuffer.setBuffer((uint8_t *)frameBuf);
//  drawCornersFull();
//  display.writeBufferDMA((uint8_t *)frameBuf, VIDEO_W * VIDEO_H * 2);
//#else
  display.writeBufferDMA((uint8_t *)STORAGE_ERROR_SPLASH, VIDEO_W * VIDEO_H * 2);
//#endif
}

void  displayNoVideosFound() {
  dbgPrint("No Videos Found!");
  waitForScreenDMA();
  setScreenAddressWindow(VIDEO_X, VIDEO_Y, VIDEO_W - 1, VIDEO_H - 1);
//#if DOUBLE_BUFFER
//  memcpy((uint8_t*)frameBuf, (uint8_t*)FILE_NOT_FOUND_SPLASH, VIDEO_W * VIDEO_H * 2);
//  newJPEGFrameSize(VIDEO_W, VIDEO_H);
//  screenBuffer.setWidth(VIDEO_W);
//  screenBuffer.setBuffer((uint8_t *)frameBuf);
//  drawCornersFull();
//  display.writeBufferDMA((uint8_t *)frameBuf, VIDEO_W * VIDEO_H * 2);
//#else
  display.writeBufferDMA((uint8_t *)FILE_NOT_FOUND_SPLASH, VIDEO_W * VIDEO_H * 2);
//#endif
}

#ifdef has_USB_MSC
void displayUSBMSCmessage() {
  waitForScreenDMA();
  setScreenAddressWindow(VIDEO_X, VIDEO_Y, VIDEO_W - 1, VIDEO_H - 1);
//#if DOUBLE_BUFFER
//  memcpy((uint8_t*)frameBuf, (uint8_t*)MASS_STORAGE_SPLASH, VIDEO_W * VIDEO_H * 2);
//  newJPEGFrameSize(VIDEO_W, VIDEO_H);
//  screenBuffer.setWidth(VIDEO_W);
//  screenBuffer.setBuffer((uint8_t *)frameBuf);
//  drawCornersFull();
//  display.writeBufferDMA((uint8_t *)frameBuf, VIDEO_W * VIDEO_H * 2);
//#else
  display.writeBufferDMA((uint8_t *)MASS_STORAGE_SPLASH, VIDEO_W * VIDEO_H * 2);
//#endif
}
#endif
