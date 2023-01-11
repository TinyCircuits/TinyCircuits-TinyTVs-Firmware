#include <JPEGDEC.h>                // minor customization
#include "FileNotFoundSplash.hpp"
#include "PlaybackErrorSplash.hpp"
#include "StorageErrorSplash.hpp"
#include "NoCardSplash.hpp"


#ifdef TinyTVMini
const int VIDEO_X = 0;
const int VIDEO_Y = 0;
const int VIDEO_W = 64;
const int VIDEO_H = 64;
#else
const int VIDEO_X = 24;
const int VIDEO_Y = 0;
const int VIDEO_W = 216;
const int VIDEO_H = 135;
#endif

// Effects need the framebuffer declared first
uint16_t frameBuf[VIDEO_W * VIDEO_H];
// uraster does text rendering on top of the video
uraster::frame fbFrame;
uraster::renderer2d renderer(fbFrame);
//FONT_INFO* font = thinPixel7_10ptFontInfo;



void initializeDisplay() {
  // Initialize TFT
  tft.begin();
  tft.setRotation(1);
  //tft.fillScreen(0);
  tft.setAddrWindow(0, 0, VIDEO_X + VIDEO_W, VIDEO_Y + VIDEO_H);
  tft.pushColor(0x0000, (VIDEO_X + VIDEO_W) * (VIDEO_Y + VIDEO_H));
  tft.setAddrWindow(VIDEO_X, VIDEO_Y, VIDEO_W, VIDEO_H);
  tft.setSwapBytes(true);
  tft.initDMA();
  tft.startWrite();

  // Set up target frame for renderer
  fbFrame.bufPtr = &frameBuf[0];
  fbFrame.xs = VIDEO_W;
  fbFrame.ys = VIDEO_H;


  renderer.target->fillBuf(uraster::color(0, 0, 0));
  tft.pushPixelsDMA(frameBuf, VIDEO_W * VIDEO_H);

#ifdef TinyTVMini
  digitalWrite(9, HIGH);
#else
  digitalWrite(9, LOW);
#endif
}

// JPEG callback is a framebuffer blit
int JPEGDraw(JPEGDRAW* block) {
  // Check that the block is within bounds of screen, otherwise, don't draw it
  if (block->x < VIDEO_W && block->y < VIDEO_H) {
    for (int bx = 0; bx < block->iWidth; bx++) {
      for (int by = 0; by < block->iHeight; by++) {
        int x = block->x + bx;
        int y = block->y + by;

        // Check that the pixel within the block is within screen bounds and then draw
        if (x < VIDEO_W && y < VIDEO_H) {
          int bufferIndex = y * VIDEO_W + x;
          int blockPixelIndex = by * block->iWidth + bx;
          frameBuf[bufferIndex] = ((uint16_t*)block->pPixels)[blockPixelIndex];
        }
      }
    }
  }

  return 1;
}



volatile bool frameReady[2] = {false, false};
volatile bool frameDecoded[2] = {true, true};
volatile int decoderDataLength[2] = {0, 0};
volatile uint8_t currentWriteBuf = 0;
volatile uint8_t currentDecodeBuf = 0;
volatile uint32_t lastBufferAssignment = 0;

void resetBuffers() {
  frameReady[0] = false; frameReady[1] = false;
  frameDecoded[0] = true; frameDecoded[1] = true;
  cdc.println("resetBuffers");
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

void JPEGBufferFilled(int length) {
  decoderDataLength[currentWriteBuf] = length;
  frameDecoded[currentWriteBuf] = false;
  frameReady[currentWriteBuf] = true;
  currentWriteBuf = 1 - currentWriteBuf;
}

uint8_t * getFilledJPEGBuffer() {
  //  if (frameReady[0]) {
  //    currentDecodeBuf = 0;
  //    return videoBuf[0];
  //  } else if (frameReady[1]) {
  //    currentDecodeBuf = 1;
  //    return videoBuf[1];
  //  }
  if (frameReady[1 - currentWriteBuf]) {
    currentDecodeBuf = 1 - currentWriteBuf;
    return videoBuf[1 - currentWriteBuf];
  }
  return NULL;
}

int getJPEGBufferLength() {
  return decoderDataLength[currentDecodeBuf];
}

void JPEGBufferStartDecode() {
  //frameReady[currentDecodeBuf] = false;
}

void JPEGBufferDecoded() {
  decoderDataLength[currentDecodeBuf] = 0;
  frameReady[currentDecodeBuf] = false;
  frameDecoded[currentDecodeBuf] = true;
}




int decodeJPEGIfAvailable() {
  if (!getFilledJPEGBuffer())
    return 1;

  JPEGBufferStartDecode();
  if (!jpeg.openRAM((uint8_t *)getFilledJPEGBuffer(), getJPEGBufferLength(), JPEGDraw))
  {
    dbgPrint("Could not open frame from RAM!");
  }
  jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
  jpeg.setMaxOutputSize(2048);
  jpeg.decode(0, 0, 0); // Weakest link and largest overhead

  JPEGBufferDecoded();

  return 0;
}






void core2Loop(){
  if(effects.processStartedEffects(frameBuf, VIDEO_W, VIDEO_H)){
    if (soundVolume != 0) playWhiteNoise = true;
    tft.pushPixelsDMA(frameBuf, VIDEO_W * VIDEO_H);
    tft.dmaWait();
  }else{
    playWhiteNoise = false;

    if(TVscreenOffMode){
      if(!backlightTurnedOff){
        // Turn backlight off
        #ifdef TinyTVMini
        #else
          digitalWrite(9, HIGH);
          backlightTurnedOff = true;
        #endif
      }
      return;
    }

    if (!streamer.live && decodeJPEGIfAvailable()) {
      return;
    }

    char buf[48];
    // Render stylizations
    if (showChannelNumber && !streamer.live) {
      if ( showChannelTimer ) {
        sprintf(buf, "CH%.2i", channelNumber);
        if (VIDEO_H > 64) {
          renderer.drawStr( VIDEO_W - 50, 10, buf, uraster::color(255, 255, 255), liberationSansNarrow_14ptFontInfo);
        } else {
          renderer.drawStr(VIDEO_W - 25, 5, buf, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
        }
        showChannelTimer--;
      }
    }




    if (!streamer.live && timeStamp && autoplayMode != 2)
    {
      uint64_t _t = ((millis() - tsMillisInitial));
      int h = (_t / 3600000);
      int m = (_t / 60000) % 60;
      int s = (_t / 1000) % 60;
      sprintf(buf, "%.2i:%.2i:%.2i", h, m, s);
      if (VIDEO_H > 64) {
        renderer.drawStr(16, VIDEO_H - 20, buf, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
      }else{
        renderer.drawStr(12, VIDEO_H - 10, buf, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
      }
    }

    if (showVolumeTimer > 0)
    {
      char volumeString[] = "|---------|";
      volumeString[1 + (soundVolume * 8) / 255] = '+';
      if (timeStamp) {
        if (VIDEO_H > 64) {
          renderer.drawStr(VIDEO_W - strlen(volumeString) * 7, VIDEO_H - 20, volumeString, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
        } else {
          renderer.drawStr((VIDEO_W / 2) - 28, VIDEO_H - 18, volumeString, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
        }
      } else {
        if (VIDEO_H > 64) {
          renderer.drawStr((VIDEO_W / 2) - 20, VIDEO_H - 25, volumeString, uraster::color(255, 255, 255), liberationSansNarrow_14ptFontInfo);
        } else {
          renderer.drawStr((VIDEO_W / 2) - 28, VIDEO_H - 15, volumeString, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
        }
      }
      showVolumeTimer--;
    }

    if(streamer.live){
      streamer.decode(videoBuf[0], videoBuf[1], frameBuf, JPEGDraw);
      effects.cropCorners(frameBuf, VIDEO_W, VIDEO_H);
      writeScreenBuffer();
      return;
    }

    writeScreenBuffer();
    
  }
}



void writeScreenBuffer() {
  effects.cropCorners(frameBuf, VIDEO_W, VIDEO_H);
  tft.setAddrWindow(VIDEO_X, VIDEO_Y, VIDEO_W, VIDEO_H);
  tft.pushPixelsDMA(frameBuf, VIDEO_W * VIDEO_H);
  //tft.dmaWait();
}

void  displayPlaybackError(char * filename) {
  dbgPrint("Playback error: " + String(filename));
  tft.pushPixelsDMA((uint16_t*)PlaybackErrorSplash, VIDEO_W * VIDEO_H);
}

void  displayCardNotFound() {
  dbgPrint("Card not found!");
  tft.pushPixelsDMA((uint16_t*)NoCardSplash, VIDEO_W * VIDEO_H);
}

void  displayFileSystemError() {
  dbgPrint("Filesystem Error!");
  tft.pushPixelsDMA((uint16_t*)StorageErrorSplash, VIDEO_W * VIDEO_H);
}

void  displayNoVideosFound() {
  dbgPrint("Filesystem Error!");
  tft.pushPixelsDMA((uint16_t*)FileNotFoundSplash, VIDEO_W * VIDEO_H);
  delay(10);
}



void displayUSBMSCmessage() {
  renderer.target->fillBuf(uraster::color(0, 0, 0));
  if(VIDEO_H > 64){
    renderer.drawStr(85, 45, "USB Mode", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
    renderer.drawStr(85, 55, "Eject or", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
    renderer.drawStr(85, 65, "disconnect", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
    renderer.drawStr(85, 75, "to continue", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
  }else{
    renderer.drawStr(5, 10, "USB Mode", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
    renderer.drawStr(5, 20, "Eject or", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
    renderer.drawStr(5, 30, "disconnect", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
    renderer.drawStr(5, 40, "to continue", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
  }
  tft.pushPixelsDMA(frameBuf, VIDEO_W * VIDEO_H);
}


void clearDisplay() {
  renderer.target->fillBuf(uraster::color(0, 0, 0));
  tft.dmaWait();
  tft.pushPixelsDMA(frameBuf, VIDEO_W * VIDEO_H);
}
