#include "JPEGStreamer.h"

extern const void dbgPrint(const char* s);
extern const void dbgPrint(const String& s);

JPEGStreamer::JPEGStreamer(JPEGDEC *_jpeg, uint8_t _tinyTVType){
  jpeg = _jpeg;
  tinyTVType = _tinyTVType;
}

void JPEGStreamer::stopBufferFilling(){
  // Need to reset frameSize to 0 to ensure assigned to next frame size
  frameSize = 0;
  frameDeliminatorAcquired = false;
}


uint8_t JPEGStreamer::commandCheck(uint8_t *jpegBuffer){
  // "0x30 0x30 0x64 0x63" is the start of an avi frame
  if(jpegBuffer[0] == 0x30 && jpegBuffer[1] == 0x30 && jpegBuffer[2] == 0x64 && jpegBuffer[3] == 0x63){
    frameDeliminatorAcquired = true;
    return COMMAND_TYPE::FRAME_DELIMINATOR;
    
  }else if(jpegBuffer[4] == 'T' && jpegBuffer[5] == 'Y' && jpegBuffer[6] == 'P' && jpegBuffer[7] == 'E'){
    if(tinyTVType == 0){
      dbgPrint("TV2");
    }else if(tinyTVType == 1){
      dbgPrint("TVMINI");
    }else if(tinyTVType == 2){
      dbgPrint("TVROUND");
    }
    return COMMAND_TYPE::TINYTV_TYPE;
  }

  return COMMAND_TYPE::NONE;
}


void JPEGStreamer::commandSearch(uint8_t *jpegBuffer){
  while(true){
    // Move all bytes from right (highest index) to left (lowest index) in buffer
    jpegBuffer[0] = jpegBuffer[1];
    jpegBuffer[1] = jpegBuffer[2];
    jpegBuffer[2] = jpegBuffer[3];
    jpegBuffer[3] = jpegBuffer[4];
    jpegBuffer[4] = jpegBuffer[5];
    jpegBuffer[5] = jpegBuffer[6];
    jpegBuffer[6] = jpegBuffer[7];
    jpegBuffer[7] = 0;//cdc->read();

    if(commandCheck(jpegBuffer) == COMMAND_TYPE::FRAME_DELIMINATOR){
      break;
    }
  }
}


bool JPEGStreamer::fillBuffer(uint8_t *jpegBuffer, const uint16_t jpegBufferSize, uint16_t &jpegBufferReadCount){
  return true;
}

extern int IMG_XOFF, IMG_YOFF, IMG_W, IMG_H, HW_VIDEO_W, HW_VIDEO_H;
extern bool live;
extern TinyScreen display;

void JPEGStreamer::decode(uint8_t *jpegBuffer, const uint16_t &jpegBufferReadCount, JPEG_DRAW_CALLBACK *pfnDraw){
  // Open and decode in memory
  if (!jpeg->openRAM(jpegBuffer, jpegBufferReadCount, pfnDraw)){
    dbgPrint("Could not open frame from RAM! Error: ");
    dbgPrint(String(jpeg->getLastError()));
    dbgPrint("See https://github.com/bitbank2/JPEGDEC/blob/master/src/JPEGDEC.h#L83");
  }
  if(IMG_W != jpeg->getWidth() || IMG_H != jpeg->getHeight())
  {
    //display.clearScreen();
    IMG_XOFF = (HW_VIDEO_W-jpeg->getWidth())/2;
    IMG_YOFF = (HW_VIDEO_H-jpeg->getHeight())/2;
    IMG_W = jpeg->getWidth();
    IMG_H = jpeg->getHeight();
  }
  jpeg->setPixelType(RGB565_BIG_ENDIAN);
  jpeg->setMaxOutputSize(2048);
  jpeg->decode(0, 0, 0);

  // Reset now that the frame is decoded
  //jpegBufferReadCount = 0;
}
