#include "JPEGStreamer.h"

extern const void dbgPrint(const char* s);
extern const void dbgPrint(const String& s);

JPEGStreamer::JPEGStreamer(JPEGDEC *_jpeg){
  jpeg = _jpeg;
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
}
