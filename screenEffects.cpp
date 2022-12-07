#include "screenEffects.h"


ScreenEffects::ScreenEffects(uint8_t tinyTVType) : StaticEffects(tinyTVType){

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



StaticEffects::StaticEffects(uint8_t tinyTVType) : RoundCornerEffect(tinyTVType){

}


void StaticEffects::startChangeChannelEffect(){
  currentStartedEffect = StaticEffects::CHANGE_CHANNEL;
}


void StaticEffects::startTurnOffEffect(){
  currentStartedEffect = StaticEffects::TURN_OFF;
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


// Three phases
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
    changeChannelFrameIndex = 0;
    currentStartedEffect = StaticEffects::NONE;
  }
}


void StaticEffects::processTurnOffEffect(uint16_t *screenBuffer, uint8_t width, uint8_t height){
  
}