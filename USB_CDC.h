enum COMMAND_TYPE{
    NONE,
    FRAME_DELIMINATOR,
    TINYTV_TYPE
};


static uint16_t readCount;
uint32_t frameSize = 0;
bool frameDeliminatorAcquired = false;
uint32_t liveTimeoutStart = 0;
uint16_t liveTimeoutLimitms = 750;

void JPEGBufferFilled(int);


uint8_t commandCheck(uint8_t *jpegBuffer){
  // "0x30 0x30 0x64 0x63" is the start of an avi frame
  if(jpegBuffer[0] == 0x30 && jpegBuffer[1] == 0x30 && jpegBuffer[2] == 0x64 && jpegBuffer[3] == 0x63){
    frameDeliminatorAcquired = true;
    return FRAME_DELIMINATOR;
    
  }else if(jpegBuffer[4] == 'T' && jpegBuffer[5] == 'Y' && jpegBuffer[6] == 'P' && jpegBuffer[7] == 'E'){
    #if !defined(TinyTVKit) && !defined(TinyTVMini)
      cdc.write("TV2");
    #elif !defined(TinyTVKit)
      cdc.write("TVMINI");
    #else
      SerialUSB.write("TVDIY");
    #endif
    return TINYTV_TYPE;
  }else if(jpegBuffer[5] == 'V' && jpegBuffer[6] == 'E' && jpegBuffer[7] == 'R'){
    // Allow for major.minor.patch up to [XXX.XXX.XXX]
    char version[12];
    sprintf(version, "[%u.%u.%u]", MAJOR, MINOR, PATCH);

    #if defined(TINYTV_2_COMPILE) || defined(TINYTV_MINI_COMPILE)
      cdc.write(version);
    #elif defined(TINYTV_KIT_COMPILE)
      SerialUSB.write(version);
    #endif
  }

  return NONE;
}


void commandSearch(uint8_t *jpegBuffer){
  #ifdef TinyTVKit
    while(SerialUSB.available()){
      // Move all bytes from right (highest index) to left (lowest index) in buffer
      jpegBuffer[0] = jpegBuffer[1];
      jpegBuffer[1] = jpegBuffer[2];
      jpegBuffer[2] = jpegBuffer[3];
      jpegBuffer[3] = jpegBuffer[4];
      jpegBuffer[4] = jpegBuffer[5];
      jpegBuffer[5] = jpegBuffer[6];
      jpegBuffer[6] = jpegBuffer[7];
      jpegBuffer[7] = SerialUSB.read();

      if(commandCheck(jpegBuffer) == FRAME_DELIMINATOR){
        break;
      }
    }
  #else
    while(cdc.available()){
      // Move all bytes from right (highest index) to left (lowest index) in buffer
      jpegBuffer[0] = jpegBuffer[1];
      jpegBuffer[1] = jpegBuffer[2];
      jpegBuffer[2] = jpegBuffer[3];
      jpegBuffer[3] = jpegBuffer[4];
      jpegBuffer[4] = jpegBuffer[5];
      jpegBuffer[5] = jpegBuffer[6];
      jpegBuffer[6] = jpegBuffer[7];
      jpegBuffer[7] = cdc.read();

      if(commandCheck(jpegBuffer) == FRAME_DELIMINATOR){
        break;
      }
    }
  #endif
}


#ifndef TINYTV_KIT_COMPILE
bool incomingCDCHandler(uint8_t *jpegBuffer, const uint16_t jpegBufferSize, uint16_t &jpegBufferReadCount = readCount){
  if(cdc.available() > 0){
    liveTimeoutStart = millis();

    if(frameDeliminatorAcquired){
      live = true;
      //return fillBuffer(jpegBuffer, jpegBufferSize, jpegBufferReadCount);
      if(frameSize == 0){
        frameSize = (((uint16_t)jpegBuffer[7]) << 24) | (((uint16_t)jpegBuffer[6]) << 16) | (((uint16_t)jpegBuffer[5]) << 8) | ((uint16_t)jpegBuffer[4]);

        if(frameSize >= jpegBufferSize){
          frameSize = 0;
          //jpegBufferReadCount = 0;
          frameDeliminatorAcquired = false;
          cdc.println("ERROR: Received frame size is too big, something went wrong, searching for frame deliminator...");
        }
      }else{
        // If the frame size was determined, get number of bytes to read, check if done filling, then fill if not done
        uint16_t bytesToReadCount = frameSize - (jpegBufferReadCount+1);

        if(bytesToReadCount <= 0){
          frameSize = 0;
          frameDeliminatorAcquired = false;
          JPEGBufferFilled(jpegBufferReadCount);
          jpegBufferReadCount = 0;
          return true;
        }

        // Just in case, check if this read will take us out of bounds, if so, restart (shouldn't happen except for future changes forgetting about this)
        if(jpegBufferReadCount+bytesToReadCount < frameSize){
          jpegBufferReadCount += cdc.read(jpegBuffer + jpegBufferReadCount, bytesToReadCount);
        }else{
          // Not going to get reset by decode so do it here so it doesn't get stuck out of bounds forever
          jpegBufferReadCount = 0;

          frameSize = 0;
          frameDeliminatorAcquired = false;
          cdc.println("ERROR: Tried to place jpeg data out of bounds...");
        }
      }
      if (millis() - liveTimeoutStart >= liveTimeoutLimitms) // Do timeout check if we're waiting for data
      {
        frameSize = 0; 
        frameDeliminatorAcquired = false;
        live = false;
      }
      // Buffer not filled yet, wait for more bytes
      return false;
    }else{
      // Search for deliminator to get back to filling buffers or respond to commands
      commandSearch(jpegBuffer);
    }
  }else if(millis() - liveTimeoutStart >= liveTimeoutLimitms){
    // A timeout is a time to reset states of both jpeg buffers, reset everything
    frameSize = 0; 
    frameDeliminatorAcquired = false;
    live = false;

    // Wait for decoding to finish and then reset incoming jpeg data read counts (otherwise may start at something other than zero next time)
  }

  // No buffer filled, wait for more bytes
  return false;
}
#endif