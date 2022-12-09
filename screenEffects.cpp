#include "screenEffects.h"


ScreenEffects::ScreenEffects(uint8_t width, uint8_t height) : StaticEffects(width, height){

}



RoundCornerEffect::RoundCornerEffect(uint8_t width, uint8_t height){
  screenWidth = width;
  screenHeight = height;

  // The max number of limits is the height of the screen (one for each row of pixels)
  cropRadiusLimitsSize = height/2;
  cropRadiusLimits = (uint8_t *)malloc(cropRadiusLimitsSize * sizeof(uint8_t));

  // Calculate limits with default cropRadius at first
  calculateLimits();
}


void RoundCornerEffect::setCropRadius(uint8_t radius){
  lastCropRadius = cropRadius;
  cropRadius = radius;
  calculateLimits();
}


void RoundCornerEffect::calculateLimits(){
  // Starting from top-left move down and right each time until reach inside of circle, that's the limit
  for (uint8_t y=0; y<cropRadius && y<screenHeight/2; y++){
    uint8_t x = 0;
    while (x < cropRadius+1){
      if (sqrt(pow(x - cropRadius, 2) + pow(y - cropRadius, 2)) <= cropRadius){
        cropRadiusLimits[y] = x;
        break;
      }
      x++;
    }
  }
}


void RoundCornerEffect::cropCorners(uint16_t *screenBuffer, uint8_t width, uint8_t height){
  for(uint8_t y=0; y<cropRadius; y++){
    for(uint8_t x=0; x<cropRadiusLimits[y]; x++){
      uint16_t topLeftBufferIndex = y * width + x;
      uint16_t topRightBufferIndex = y * width + ((width-1) - x);

      uint16_t bottomLeftBufferIndex = ((height-1) - y) * width + x;
      uint16_t bottomRightBufferIndex = ((height-1) - y) * width + ((width-1) - x);

      screenBuffer[topLeftBufferIndex] = 0;
      screenBuffer[topRightBufferIndex] = 0;
      screenBuffer[bottomLeftBufferIndex] = 0;
      screenBuffer[bottomRightBufferIndex] = 0;
    }
  }
}



StaticEffects::StaticEffects(uint8_t width, uint8_t height) : RoundCornerEffect(width, height){
  turnOffRadiusLimits = (uint8_t*)malloc(height * sizeof(uint8_t)); 
}


void StaticEffects::startChangeChannelEffect(){
  changeChannelFrameIndex = 0;
  currentStartedEffect = StaticEffects::CHANGE_CHANNEL;
}


void StaticEffects::startTurnOffEffect(){
  if(screenHeight > 64){
    turnOffRadius = 120;
  }else{
    turnOffRadius = 35;
  }
  currentStartedEffect = StaticEffects::TURN_OFF;
}


void StaticEffects::stopEffects(){
  currentStartedEffect = StaticEffects::NONE;
}


bool StaticEffects::processStartedEffects(uint16_t *screenBuffer, uint8_t width, uint8_t height){
  switch(currentStartedEffect){
    case StaticEffects::CHANGE_CHANNEL:
      processChangeChannelEffect(screenBuffer, width, height);
      return true;
    break;
    case StaticEffects::TURN_OFF:
      processTurnOffEffect(screenBuffer, width, height);
      return true;
    break;
  }
  return false;
}


void StaticEffects::makeStaticEffectFrame(uint16_t *screenBuffer, uint8_t width, uint8_t height){
  uint32_t staticPos = 0;
  while (staticPos < width * height){
    uint8_t currentRand = rand();
    uint8_t currentRandSmall = ((currentRand >> 6 - (rand()) / 2)) & 3;
    if (currentRandSmall == 3) {
      ((uint16_t *)screenBuffer)[staticPos] = 0x0861;//black
    } else if (currentRandSmall == 2) {
      ((uint16_t *)screenBuffer)[staticPos] = 0xF79E;//white
    } else if (currentRandSmall == 1) {
      ((uint16_t *)screenBuffer)[staticPos] = 0x8410;//black/grey
    }
    staticPos += (currentRand & 3) + 1;
  }
}


void StaticEffects::processChangeChannelEffect(uint16_t *screenBuffer, uint8_t width, uint8_t height){
  // If at index 0, fill the buffer with a base layer of noise
  // using the existing pixels to create a sort of transistion,
  // else, fill with static or gray and then static
  if(changeChannelFrameIndex == 0){
    for (uint16_t i=0; i<width*height; i++){
      // Another magic color from original firmware
      screenBuffer[i] &= 0b1110011110011100;
      screenBuffer[i] >>= 2;
    }
  }else{
    if(changeChannelFrameIndex >= 1){
      for (uint16_t i=0; i<width*height; i++){
        // Change to magic number color taken from original firmware
        screenBuffer[i] = (0x8410 & 0b1100011100011000) >> 3;
      }
    }

    // Run the static effect on top of the base noise layer/buffer, 5 times
    for(uint8_t i=0; i<5; i++){
      makeStaticEffectFrame(screenBuffer, width, height);
    }
  }
  cropCorners(screenBuffer, width, height);

  changeChannelFrameIndex++;

  // End the effect after enough frames (enough meaning, what looks good as an effect)
  if(changeChannelFrameIndex >= changeChannelFrameCount){
    currentStartedEffect = StaticEffects::NONE;
  }
}


void StaticEffects::processTurnOffEffect(uint16_t *screenBuffer, uint8_t width, uint8_t height){
  int xCircle = turnOffRadius/2;
  int yCircle = 0;
  int radiusError = 1-xCircle;
  uint8_t halfHeight = height/2;

  // Calculate radius limits for this value of 'turnOffRadius'
  memset(turnOffRadiusLimits, 0, (height*sizeof(uint8_t)));
  while(xCircle >= yCircle){
    turnOffRadiusLimits[halfHeight + yCircle] = xCircle * 3 / 2;
    turnOffRadiusLimits[halfHeight - yCircle] = xCircle * 3 / 2;
    turnOffRadiusLimits[halfHeight - xCircle] = yCircle * 3 / 2;
    turnOffRadiusLimits[halfHeight + xCircle] = yCircle * 3 / 2;
    yCircle++;
    if (radiusError < 0){
      radiusError += 2 * yCircle + 1;
    }else{
      xCircle--;
      radiusError += 2 * (yCircle - xCircle) + 1;
    }
  }

  // Generate static between radius limits
  for (int y = 0; y < height; y++){
    memset(screenBuffer+(y*width), 0, (width*sizeof(uint16_t)));
    if (turnOffRadiusLimits[y]){
      uint8_t min = (width/2) - turnOffRadiusLimits[y];
      uint8_t max = (width/2) + turnOffRadiusLimits[y];
      for(uint8_t x=min; x<max; x++){
        uint8_t currentRand = rand();
        uint8_t currentRandSmall = ((currentRand >> 4) & 3);
        if(x == width / 2 - turnOffRadiusLimits[y] - 3)
          x += (currentRand & 3);
        if(currentRandSmall == 3){
          screenBuffer[(y*width)+x] = 0x0861;//black
        }else if (currentRandSmall == 2){
          screenBuffer[(y*width)+x] = 0xF79E;//white
        }else if (currentRandSmall == 1){
          screenBuffer[(y*width)+x] = 0x8410;//black/grey
        }
        x += (currentRand & 3) * 2;
      }
    }
  }

  // Decrease the radius size to make it get smaller as time goes on
  turnOffRadius -= turnOffRadius / 8;

  // Generate rectangular 'blip' when radius gets very small
  if(turnOffRadius < 12){
    turnOffRadius--;
    if(turnOffRadius < 6 && turnOffRadius > 3 ){
      for(int y = height / 2 - 2; y < height / 2 + 1; y++){
        memset(screenBuffer+(y*width), 0, (width * sizeof(uint16_t)));
        for(int x = width / 2 - (width/8); x < width / 2 + (width/8); x++){
          uint8_t currentRand = rand();
          uint8_t currentRandSmall = ((currentRand >> 4) & 3);
          
          if(x == width / 2 - turnOffRadiusLimits[y] - 3){
            x += (currentRand & 3);
          }

          if(currentRandSmall == 3){
            screenBuffer[(y*width)+x] = 0x0861;
          }else if (currentRandSmall == 2){
            screenBuffer[(y*width)+x] = 0xF79E;
          }else if (currentRandSmall == 1){
            screenBuffer[(y*width)+x] = 0x8410;
          }
          x += (currentRand & 3) * 2;
        }
      }
    }

    // Vary the speed of the blip as it gets smaller (looks more "dynamic")
    if(turnOffRadius < 6){
      delay((6 - turnOffRadius) * 15);
    }
  }

  // End the effect
  if(turnOffRadius <= 0){
    currentStartedEffect = StaticEffects::NONE;
  }
}