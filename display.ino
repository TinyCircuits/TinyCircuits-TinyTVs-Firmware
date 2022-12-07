#include <JPEGDEC.h>                // minor customization

JPEGDEC jpeg;
TFT_eSPI tft = TFT_eSPI();

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
  tft.fillScreen(0);  // Fill entire screen to black to overwrite all potentially unmodified pixels
  tft.setAddrWindow(VIDEO_X, VIDEO_Y, VIDEO_W, VIDEO_H);
  //tft.pushColor(0x0000, VIDEO_W * VIDEO_H);
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
int JPEGDraw(JPEGDRAW* pDraw);
int JPEGDraw(JPEGDRAW* block){
  // Check that the block is within bounds of screen, otherwise, don't draw it
  if(block->x < VIDEO_W && block->y < VIDEO_H){
    for (int bx = 0; bx < block->iWidth; bx++){
      for (int by = 0; by < block->iHeight; by++){
        int x = block->x + bx;
        int y = block->y + by;

        // Check that the pixel within the block is within screen bounds and then draw
        if(x < VIDEO_W && y < VIDEO_H){
          int bufferIndex = y * VIDEO_W + x;
          int blockPixelIndex = by * block->iWidth + bx;
          frameBuf[bufferIndex] = ((uint16_t*)block->pPixels)[blockPixelIndex];
        }
      }
    }
  }

  return 1;
}




void core2Loop()
{
  if(effects.processStartedEffects(frameBuf, VIDEO_W, VIDEO_H)){
    if (soundVolume != 0) playWhiteNoise = true;
    tft.pushPixelsDMA(frameBuf, VIDEO_W * VIDEO_H);
    tft.dmaWait();
    return;
  }else{
    playWhiteNoise = false;
  }

  char buf[48];
  // Wait for core 1 to get the compressed data ready
  while (!frameReady || TVscreenOffMode)
  {
    sleep_us(400);
    //dbgPrint("Core 2 wait spinning");
  }
  uint64_t t0 = time_us_64();
  decodingFrame = true;
  if (!jpeg.openRAM((uint8_t*)videoBuf[1 - currentWriteBuf], decoderDataLength, JPEGDraw))
  {
    dbgPrint("Could not open frame from RAM!");
  }
  jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
  jpeg.setMaxOutputSize(2048);
  tft.setAddrWindow(VIDEO_X, VIDEO_Y, VIDEO_W, VIDEO_H);

  jpeg.decode(0, 0, 0); // Weakest link and largest overhead




  // Render stylizations
  if (showChannelNumber) {
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
  if (timeStamp && autoplayMode != 2)
  {
    uint64_t _t = ((millis() - tsMillisInitial));
    int h = (_t / 3600000);
    int m = (_t / 60000) % 60;
    int s = (_t / 1000) % 60;
    sprintf(buf, "%.2i : %.2i : %.2i", h, m, s);
    renderer.drawStr(16, VIDEO_H - 20, buf, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
  }

  if (showVolumeTimer > 0)
  {
    char volumeString[] = "|---------|";
    volumeString[1 + (soundVolume * 8) / 255] = '+';
    if (timeStamp) {
      renderer.drawStr(VIDEO_W - strlen(volumeString) * 5, VIDEO_H - 20, volumeString, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
    } else {
      if (VIDEO_H > 64) {
        renderer.drawStr((VIDEO_W / 2) - 20, VIDEO_H - 25, volumeString, uraster::color(255, 255, 255), liberationSansNarrow_14ptFontInfo);
      } else {
        renderer.drawStr((VIDEO_W / 2) - 28, VIDEO_H - 15, volumeString, uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
      }
    }
    showVolumeTimer--;
  }





  uint64_t t1 = time_us_64();
  // Wait for DMA to finish and then push everything out
  effects.cropCorners(frameBuf, VIDEO_W, VIDEO_H);
  tft.dmaWait();
  tft.pushPixelsDMA(frameBuf, VIDEO_W * VIDEO_H);

  decodingFrame = false;

  // Set the frameReady flag to false so core 1 knows we need another frame to decode
  frameReady = false;

  //dbgPrint("took " + String(uint32_t(t1 - t0)) + "us");
}


void  displayCardNotFound() {
  dbgPrint("Card not found!");
  renderer.target->fillBuf(uraster::color(0, 0, 0));
  renderer.drawStr(5, 16, "Card not found!", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
  renderer.drawStr(5, 28, "Insert media and restart.", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
  tft.pushPixelsDMA(frameBuf, VIDEO_W * VIDEO_H);
}

void displayUSBMSCmessage() {
  renderer.target->fillBuf(uraster::color(0, 0, 0));
  renderer.drawStr(5, 10, "USB Mode", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
  renderer.drawStr(5, 20, "Eject or", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
  renderer.drawStr(5, 30, "disconnect", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
  renderer.drawStr(5, 40, "to continue", uraster::color(255, 255, 255), thinPixel7_10ptFontInfo);
  tft.pushPixelsDMA(frameBuf, VIDEO_W * VIDEO_H);
}














void clearDisplay() {
  renderer.target->fillBuf(uraster::color(0, 0, 0));
  tft.dmaWait();
  tft.pushPixelsDMA(frameBuf, VIDEO_W * VIDEO_H);
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

void staticEffect()
{
  uint32_t staticPos = 0;
  while (staticPos < VIDEO_W * VIDEO_H) {
    uint8_t currentRand = rand();
    uint8_t currentRandSmall = ((currentRand >> 6 - (rand()) / 2)) & 3;
    if (currentRandSmall == 3) {
      ((uint16_t *)frameBuf)[staticPos] = colorAndMask1;//black
    } else if (currentRandSmall == 2) {
      ((uint16_t *)frameBuf)[staticPos] = colorOrMask4;//white
    } else if (currentRandSmall == 1) {
      ((uint16_t *)frameBuf)[staticPos] = colorOrMask1;//black/grey
    }
    staticPos += (currentRand & 3) + 1;
  }
}

// void changeChannelEffect()
// {
//   if (soundVolume != 0) playWhiteNoise = true;
//   for (int i = 0; i < VIDEO_W * VIDEO_H; i++)
//   {
//     frameBuf[i] &= 0b1110011110011100;
//     frameBuf[i] >>= 2;
//   }
//   tft.pushPixelsDMA(frameBuf, VIDEO_W * VIDEO_H);
//   tft.dmaWait();
//   // TODO: Make more convincing
//   for (int i = 0; i < 3; i++)
//   {
//     staticEffect();
//     staticEffect();
//     staticEffect();
//     staticEffect();
//     staticEffect();
//     tft.pushPixelsDMA(frameBuf, VIDEO_W * VIDEO_H);
//     tft.dmaWait();
//     //memset(frameBuf, 0, 2*VIDEO_W*VIDEO_H);
//     for (int i = 0; i < VIDEO_W * VIDEO_H; i++) frameBuf[i] = (colorOrMask1 & 0b1100011100011000) >> 3;
//   }
// }

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
      tft.dmaWait();
      memset(staticBuf, 0, VIDEO_W * sizeof(uint16_t));
      //tft.readRect(0, y, 240, 1, staticBuf);
      //tft.setAddrWindow(0, y, 240, 1);
      for (int x = 0; x < VIDEO_W / 2 - radiusLimits[y]; x++) {
        ((uint16_t *)staticBuf)[x] = (((uint16_t *)staticBuf)[x] >> 1) & 0x39E7;
      }
      for (int x = VIDEO_W / 2 + radiusLimits[y]; x < VIDEO_W; x++) {
        ((uint16_t *)staticBuf)[x] = (((uint16_t *)staticBuf)[x] >> 1) & 0x39E7;
      }
      tft.pushPixelsDMA(staticBuf, VIDEO_W);
    }
    memset(staticBuf, 0, VIDEO_W * sizeof(uint16_t));
    for (int y = 0; y < VIDEO_H; y++) {
      tft.dmaWait();
      memset(staticBuf, 0, VIDEO_W * sizeof(uint16_t));
      //tft.readRect(0, y, 240, 1, staticBuf);
      //tft.setAddrWindow(0, y, 240, 1);
      if (radiusLimits[y]) {
        for (int x = VIDEO_W / 2 - radiusLimits[y] - 3; x < VIDEO_W / 2 + radiusLimits[y] + 3; x) {
          uint8_t currentRand = rand();
          uint8_t currentRandSmall = ((currentRand >> 4) & 3);
          if (x == VIDEO_W / 2 - radiusLimits[y] - 3)
            x += (currentRand & 3);
          if (currentRandSmall == 3) {
            ((uint16_t *)staticBuf)[x] = colorAndMask1;//black
          } else if (currentRandSmall == 2) {
            ((uint16_t *)staticBuf)[x] = colorOrMask4;//white
          } else if (currentRandSmall == 1) {
            ((uint16_t *)staticBuf)[x] = colorOrMask1;//black/grey
          }
          x += (currentRand & 3) * 2;
        }
      }
      tft.pushPixelsDMA(staticBuf, VIDEO_W);
    }
    pauseRadius -= pauseRadius / 8;
    if (pauseRadius < 12) {
      pauseRadius--;

      if (pauseRadius < 6 && pauseRadius > 3 ) {
        for (int y = VIDEO_H / 2 - 2; y < VIDEO_H / 2 + 1; y++) {
          tft.dmaWait();
          memset(staticBuf, 0, VIDEO_W * sizeof(uint16_t));
          for (int x = VIDEO_W / 2 - 30; x < VIDEO_W / 2 + 30; x++) {
            uint8_t currentRand = rand();
            uint8_t currentRandSmall = ((currentRand >> 4) & 3);
            if (x == VIDEO_W / 2 - radiusLimits[y] - 3)
              x += (currentRand & 3);
            if (currentRandSmall == 3) {
              ((uint16_t *)staticBuf)[x] = colorAndMask1;//black
            } else if (currentRandSmall == 2) {
              ((uint16_t *)staticBuf)[x] = colorOrMask4;//white
            } else if (currentRandSmall == 1) {
              ((uint16_t *)staticBuf)[x] = colorOrMask1;//black/grey
            }
            x += (currentRand & 3) * 2;
          }
          tft.setAddrWindow(VIDEO_X, y, VIDEO_W, 1);
          tft.pushPixelsDMA(staticBuf, VIDEO_W);
        }
      }
      if (pauseRadius < 6) {
        delay((6 - pauseRadius) * 15);
      }
    }
    if (pauseRadius <= 0) {
      pauseRadius = 0;
      //memset(buffer, 0, sizeof(buffer));
    }
  }
  tft.setAddrWindow(VIDEO_X, VIDEO_Y, VIDEO_W, VIDEO_H);
}
