#ifndef JPEG_STREAMER_H
#define JPEG_STREAMER_H

#include <JPEGDEC.h>
#include <stdlib.h>
#include <TinyScreen.h>

class JPEGStreamer{
  public:
    JPEGStreamer(JPEGDEC *_jpeg);

    //void decode(uint8_t *jpegBuffer0, uint8_t *jpegBuffer1, uint16_t *screenBuffer, JPEG_DRAW_CALLBACK *pfnDraw);  // Pass JPEGDec callback function (No longer necessary, one JPEG buffer)
    void decode(uint8_t *jpegBuffer, const uint16_t &jpegBufferReadCount, JPEG_DRAW_CALLBACK *pfnDraw);   // Pass JPEGDec callback function

  private:

    JPEGDEC *jpeg;
};

#endif