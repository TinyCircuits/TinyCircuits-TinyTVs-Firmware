#ifndef JPEG_STREAMER_H
#define JPEG_STREAMER_H

#include <JPEGDEC.h>
#include <stdlib.h>
#include <TinyScreen.h>

// When 'fillBuffers' and 'decode' are called on separate cores,
// incoming //Serial data gets stored in buffers and then decoded. Decoded
// buffers are written to a screen buffer in 'core1Decode' which can
// immediately be pushed to the screen.
class JPEGStreamer{
  public:
    JPEGStreamer(JPEGDEC *_jpeg, uint8_t _tinyTVType);

    //void decode(uint8_t *jpegBuffer0, uint8_t *jpegBuffer1, uint16_t *screenBuffer, JPEG_DRAW_CALLBACK *pfnDraw);  // Pass JPEGDec callback function (No longer necessary, one JPEG buffer)
    void decode(uint8_t *jpegBuffer, const uint16_t &jpegBufferReadCount, JPEG_DRAW_CALLBACK *pfnDraw);   // Pass JPEGDec callback function

  private:

    JPEGDEC *jpeg;

    // Passed in constructor, type of TV to respond with
    uint8_t tinyTVType = 0;
};

#endif