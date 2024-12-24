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



#define byteswap(x) ((uint16_t)x >> 8) | ((uint16_t)x << 8)
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


uint32_t staticStart = 0;
uint32_t staticTimer = 0;

void drawStaticFor(uint32_t timeMS) {
  staticStart = millis();
  staticTimer = timeMS;
}

void stopStaticEffect() {
  staticTimer = 0;
}

void drawStatic(uint16_t * buf, uint32_t staticWidth, uint32_t staticHeight ) {
  int ratio = staticTimer - (staticTimer - (millis() - staticStart));
  ratio = constrain(ratio, 10, 100);
  if (staticTimer > millis() - staticStart) {
    int iters = 2; //pixel density for static
    if (VIDEO_H > 64)
      iters = 5;
    for (int i = 0; i < iters; i++)
    {
      uint32_t staticPos = 0;
      while (staticPos < staticWidth * staticHeight) {
        /*
          uint8_t currentRand = rand();
          uint8_t currentRandSmall = ((currentRand >> 6 - (rand()) / 2)) & 3;
        */
        uint8_t currentRand = qrand();
        uint8_t currentRandSmall = ((currentRand >> 4) & 3);
        if (currentRandSmall == 3) {
          ((uint16_t *)buf)[staticPos] = byteswap(colorAndMask1);//black
        } else if (currentRandSmall == 2) {
          ((uint16_t *)buf)[staticPos] = byteswap(colorOrMask4);//white
        } else if (currentRandSmall == 1) {
          ((uint16_t *)buf)[staticPos] = byteswap(colorOrMask1);//black/grey
        }
        //staticPos += (currentRand & 3) + 1;
        staticPos += currentRand % ratio;
      }
    }
  }
}

uint32_t volumeDrawStart = 0;
uint32_t volumeDrawTimer = 0;

void drawVolumeFor(uint32_t timeMS) {
  volumeDrawStart = millis();
  volumeDrawTimer = timeMS;
}

void drawVolume(JPEGDRAW* block) {
  if (showVolumeBar) {
    if (volumeDrawTimer > millis() - volumeDrawStart) {
      if (VIDEO_H > 64) {
        if (block->y <= (VIDEO_H - 25) && block->y + block->iHeight > (VIDEO_H - 25)) {
          char volumeString[] = "|-------|";
          volumeString[1 + volumeSetting] = '+';
          screenBuffer.setCursor((VIDEO_W / 2) - 18 - block->x, /*(VIDEO_H - 25) - block->y*/0);
          screenBuffer.print(volumeString);
        }
      } else {
        if (block->y <= (VIDEO_H - 12) && block->y + block->iHeight > (VIDEO_H - 12)) { //was 25
          char volumeString[] = "|-------|";
          volumeString[1 + volumeSetting] = '+';
          screenBuffer.setCursor((VIDEO_W / 2) - 22 - block->x, 0/*VIDEO_H - 15 - (16 * 3)*/);
          screenBuffer.print(volumeString);
        }
      }
    }
  }
}

uint32_t channelDrawStart = 0;
uint32_t channelDrawTimer = 0;

void drawChannelNumberFor(uint32_t timeMS) {
  channelDrawStart = millis();
  channelDrawTimer = timeMS;
}

void drawChannelNumber(JPEGDRAW* block) {
  if (showChannelNumber) {
    if (channelDrawTimer > millis() - channelDrawStart) {
      char buf[10];
      sprintf(buf, "CH%.2i", channelNumber);
      if (IMG_H > 64) {
        if (block->y < 20 && block->y > 10 && block->x + block->iWidth > IMG_W - 10) {
          screenBuffer.setCursor(IMG_W - 55 - block->x, /*16 - block->y*/0);
          screenBuffer.print(buf);
        }
      } else {
        if (block->y < 8) {
          int xOffset = 25;
          if (IMG_W == 64) {
            xOffset = 32;
          }
          screenBuffer.setCursor(IMG_W - xOffset - block->x, 5 - block->y);
          screenBuffer.print(buf);
        }
      }
    }
  }
}

uint8_t cropRadiusLimits[26];

void setCornerRadius(uint8_t cropRadius) {
  for (uint8_t y = 0; y < cropRadius; y++) {
    uint8_t x = 0;
    while (x < cropRadius + 1) {
      if (sqrt(pow(x - cropRadius, 2) + pow(y - cropRadius, 2)) <= cropRadius) {
        cropRadiusLimits[y] = x;
        break;
      }
      x++;
    }
  }
}


void drawCornersPartial(JPEGDRAW* block) {
  if (roundedCorners) {
    if (IMG_H > 64 || IMG_W == 64) {
      if (block->y < 32 && block->x == 0) {
        for (int y = 0; y < block->iHeight && block->y + y < 24; y++) {
          screenBuffer.drawLine(0, y, cropRadiusLimits[block->y + y], y, 0x0000);
        }
      }
      if (block->y < 32 && block->x + block->iWidth >= IMG_W) {
        for (int y = 0; y < block->iHeight && block->y + y < 24; y++) {
          screenBuffer.drawLine(IMG_W - block->x - 0 - cropRadiusLimits[block->y + y], y, IMG_W - block->x - 1, y, 0x0000);
        }
      }
      if (block->y > IMG_H - 32 && block->x == 0) {
        for (int y = 0; y < block->iHeight; y++) {
          if (block->y + y > IMG_H - 25) {
            screenBuffer.drawLine(0, y, cropRadiusLimits[ IMG_H - (block->y + y)], y, 0x0000);
          }
        }
      }
      if (block->y > IMG_H - 32 && block->x + block->iWidth >= IMG_W) {
        for (int y = 0; y < block->iHeight && block->y + y > IMG_H - 25; y++) {
          screenBuffer.drawLine(IMG_W - block->x - 0 - cropRadiusLimits[IMG_H - (block->y + y)], y, IMG_W - block->x - 1, y, 0x0000);
        }
      }
    }
  }
}

void drawCornersFull() {
  if (roundedCorners) {
    for (int y = 0; y < 24; y++) {
      screenBuffer.drawLine(0, y, cropRadiusLimits[y], y, 0x0000);
    }
    for (int y = 0; y < 24; y++) {
      screenBuffer.drawLine(IMG_W - cropRadiusLimits[y], y, IMG_W - 1, y, 0x0000);
    }
    for (int y = 0; y < 24; y++) {
      screenBuffer.drawLine(0, IMG_H - y - 1, cropRadiusLimits[y], IMG_H - y - 1, 0x0000);
    }
    for (int y = 0; y < 24; y++) {
      screenBuffer.drawLine(IMG_W - cropRadiusLimits[y], IMG_H - y - 1, IMG_W - 1, IMG_H - y - 1, 0x0000);
    }
  }
}


// For tube off effect
int pauseRadius = 0;

void startTubeOffEffect() {
  if (VIDEO_H <= 64) {
    pauseRadius = 24;
  } else {
    pauseRadius = 56;
  }
}

int tubeOffEffect() {
  delay(5);
  if (pauseRadius) {
    int xCircle = pauseRadius / 2;
    int yCircle = 0;
    int radiusError = 1 - xCircle;
    uint8_t radiusLimits[VIDEO_H];
    uint16_t staticBuf[VIDEO_W];
    memset(radiusLimits, 0, sizeof(radiusLimits));
    while (xCircle >= yCircle) {
      radiusLimits[VIDEO_H / 2 - 2 + yCircle] = xCircle * 3 / 2;
      radiusLimits[VIDEO_H / 2 - 2 - yCircle] = xCircle * 3 / 2;
      radiusLimits[VIDEO_H / 2 - 2 - xCircle] = yCircle * 3 / 2;
      radiusLimits[VIDEO_H / 2 - 2 + xCircle] = yCircle * 3 / 2;
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
      setScreenAddressWindow(VIDEO_X, y + 1, VIDEO_W, 1);
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
      setScreenAddressWindow(VIDEO_X, y + 1, VIDEO_W, 1);
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
          setScreenAddressWindow(VIDEO_X, y, VIDEO_W, 1);
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
  waitForScreenDMA();
  setScreenAddressWindow(VIDEO_X, VIDEO_Y, VIDEO_W - 1, VIDEO_H - 1);
  return pauseRadius;
}
