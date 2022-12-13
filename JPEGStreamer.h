#ifndef JPEG_STREAMER_H
#define JPEG_STREAMER_H

#include <JPEGDEC.h>
#include <Adafruit_TinyUSB.h>
#include <stdlib.h>


// When 'fillBuffers' and 'decode' are called on separate cores,
// incoming serial data gets stored in buffers and then decoded. Decoded
// buffers are written to a screen buffer in 'core1Decode' which can
// immediately be pushed to the screen.
class JPEGStreamer{
  public:
    JPEGStreamer(JPEGDEC *_jpeg, Adafruit_USBD_CDC *_cdc, uint8_t _tinyTVType);

    // Main functions to call on separate cores
    void fillBuffers(uint8_t *jpegBuffer0, uint8_t *jpegBuffer1, const uint16_t jpegBufferSize); // Both buffers are expected to be the same size
    void decode(uint8_t *jpegBuffer0, uint8_t *jpegBuffer1, uint16_t *screenBuffer, JPEG_DRAW_CALLBACK *pfnDraw);  // Pass JPEGDec callback function, returns true if decoded
  
    // Enum values used for locking access between core 
    // for jpeg buffers 'jpegBuffer0' and 'jpegBuffer1'
    enum JPEG_BUFFER_SEMAPHORE{
        UNLOCKED,
        LOCKED_BY_CORE_0,
        LOCKED_BY_CORE_1,
        WAITING_FOR_CORE_1
    };

    enum TINYTV_TYPE{
        TINYTV_2=0,
        TINYTV_MINI=1,
        TINYTV_ROUND=2
    };

    enum COMMAND_TYPE{
        NONE,
        FRAME_DELIMINATOR,
        TINYTV_TYPE
    };

    // Set true when receive 'TYPE' command and set false 
    // when do not receive jpeg data for timeout time
    bool live = false;
  private:
    void stopBufferFilling();
    uint8_t commandCheck(uint8_t *jpegBuffer);
    void commandSearch(uint8_t *jpegBuffer);
    void noDataTimeoutHandler();
    bool fillBuffer(uint8_t *jpegBuffer, const uint16_t jpegBufferSize, uint16_t &jpegBufferReadCount);
    bool incomingCDCHandler(uint8_t *jpegBuffer, const uint16_t jpegBufferSize, uint16_t &jpegBufferReadCount);

    void decode(uint8_t *jpegBuffer, uint16_t &jpegBufferReadCount, JPEG_DRAW_CALLBACK *pfnDraw);   // Pass JPEGDec callback function

    // Flags to control access to JPEG buffers during filling and decoding
    enum JPEG_BUFFER_SEMAPHORE jpegBuffer0Semaphore = JPEG_BUFFER_SEMAPHORE::UNLOCKED;
    enum JPEG_BUFFER_SEMAPHORE jpegBuffer1Semaphore = JPEG_BUFFER_SEMAPHORE::UNLOCKED;

    JPEGDEC *jpeg;
    Adafruit_USBD_CDC *cdc;

    // Used in 'core0FillBuffers(...)'
    uint16_t jpegBuffer0ReadCount = 0;
    uint16_t jpegBuffer1ReadCount = 0;

    // The frame size as received after deliminator
    uint32_t frameSize = 0;

    // Flag set true when AVI frame found in incoming serial and it's time
    // to fill buffers. False means to look for commands or deliminator
    bool frameDeliminatorAcquired = false;

    // Passed in constructor, type of TV to respond with
    uint8_t tinyTVType = 0;

    uint32_t liveTimeoutStart = 0;
    uint16_t liveTimeoutLimitms = 750;
};

#endif