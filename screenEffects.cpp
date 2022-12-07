#include "screenEffects.h"


ScreenEffects::ScreenEffects(uint8_t tinyTVType) : RoundCornerEffect(tinyTVType){

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