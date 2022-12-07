#include "screenEffects.h"


ScreenEffects::ScreenEffects(uint8_t tinyTVType) : RoundCornerEffect(tinyTVType), StaticEffects(){

}



RoundCornerEffect::RoundCornerEffect(uint8_t tinyTVType){
  if (tinyTVType == ScreenEffects::TINYTV_TYPE::TINYTV_2){
    cropRadius = tinyTV2CropRadius;
  }else if (tinyTVType == ScreenEffects::TINYTV_TYPE::TINYTV_MINI){
    cropRadius = tinyTVMiniCropRadius;
  }

  cropRadiusLimits = (uint8_t *)malloc(cropRadius * sizeof(uint8_t));
  calculateLimits();
}


void RoundCornerEffect::calculateLimits(){
  // Starting from top-left move down and right each time until reach inside of circle, that's the limit
  for (int y=0; y<cropRadius; y++){
    int x = 0;
    while (x < cropRadius*2){
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



StaticEffects::StaticEffects(){

}


void StaticEffects::startChangeChannelEffect(){
  currentStartedEffect = StaticEffects::CHANGE_CHANNEL;
}


void StaticEffects::startTurnOffEffect(){
  currentStartedEffect = StaticEffects::TURN_OFF;
}


void StaticEffects::processStartedEffects(uint16_t *screenBuffer, uint8_t width, uint8_t height){
  switch(currentStartedEffect){
    case StaticEffects::CHANGE_CHANNEL:
      processChangeChannelEffect(screenBuffer, width, height);
    break;
    case StaticEffects::TURN_OFF:
      processTurnOffEffect(screenBuffer, width, height);
    break;
  }
}


void StaticEffects::makeStaticEffectFrame(uint16_t *screenBuffer, uint8_t width, uint8_t height){
  uint32_t staticPos = 0;
  while (staticPos < width * height) {
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
  
}


void StaticEffects::processTurnOffEffect(uint16_t *screenBuffer, uint8_t width, uint8_t height){
  
}