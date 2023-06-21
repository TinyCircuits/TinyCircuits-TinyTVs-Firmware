


uint16_t jpegBufferReadCount = 0;
uint32_t frameSize = 0;
uint32_t tempFrameSize = 0;
bool frameDeliminatorAcquired = false;
uint32_t liveTimeoutStart = 0;
uint16_t liveTimeoutLimitms = 750;
char commandBuffer[30];
uint8_t commandBufPos = 0;
uint32_t commandStartMS = 0;
uint32_t commandStartTimeoutMS = 20;



#ifdef ARDUINO_ARCH_RP2040
#define SerialInterface cdc
#else
#define SerialInterface SerialUSB
#endif

//
//{"asdf":"jkl"}
//{"SET":"channel=2"}
//{"SET":"volume=1"}
//{"GET":"channel"}
//{"FRAME":5000}
//{"SET":"alphabetize=true"}
//{"GET":"alphabetize"}
//#define cdc SerialUSB

bool setKeyValue(String);
String getKeyValue(String);

void handleCDCcommand(String input) {
  if ( input.length() > 5 && input.indexOf(":") >= 0) {
    String key = input.substring(0, input.indexOf(":"));
    String val = input.substring(input.indexOf(":") + 1);
    key.trim();
    val.trim();
    while (key.indexOf("\"") >= 0) {
      key.remove(key.indexOf("\""), 1);
    }
    while (val.indexOf("\"") >= 0) {
      val.remove(val.indexOf("\""), 1);
    }
    //    cdc.print("key = ");
    //    cdc.print(key);
    //    cdc.print(", val = ");
    //    cdc.println(val);
    if (key == String("FRAME")) {
      //#ifdef ARDUINO_ARCH_RP2040



      frameDeliminatorAcquired = true;
      frameSize = val.toInt();








      //#endif
    } else if (key == String("GET")) {
      //cdc.println( String("{\"") + val + String("\":\"") + getKeyValue(val) + String("\"}"));
      SerialInterface.println( String("{\"") + val + String("\":") + getKeyValue(val) + String("}"));
      //cdc.print(val);
      //cdc.print(":");
      //cdc.println(getKeyValue(val));
    } else if (key == String("SET")) {
      if (setKeyValue(val)) {
        inputFlags.settingsChanged = true;//.saveSettings();
      }
    } else {
      SerialInterface.println("Unhandled JSON key");
    }
  } else {
    SerialInterface.print("Invalid JSON? ");
    SerialInterface.println(input);
  }
}


void commandSearch(uint16_t jpegBufferSize) {
  while (SerialInterface.available()) {
    char c = SerialInterface.read();
    if (!commandStartMS) {
      if (c == '{') {
        //start of JSON string
        commandStartMS = millis();
        commandBufPos = 0;
      }
    } else {
      if (c == '}') {
        //end of JSON string
        commandBuffer[commandBufPos] = 0;
        handleCDCcommand(String(commandBuffer));
        commandStartMS = 0;
        return;
      } else {
        //within JSON string, add bytes or wait for timeout
        if (commandBufPos < sizeof(commandBuffer) - 1) {
          commandBuffer[commandBufPos] = c;
          commandBufPos++;
        }
        if (millis() - commandStartMS > commandStartTimeoutMS) {
          commandStartMS = 0;
        }
      }
    }
  }
}


bool incomingCDCHandler(uint8_t *jpegBuffer, uint16_t jpegBufferSize, bool *live, uint16_t *totalBytes) {
  if (SerialInterface.available() > 0) {
    liveTimeoutStart = millis();

    if (!frameDeliminatorAcquired) {
      // Search for deliminator to get back to filling buffers or respond to commands
      commandSearch(jpegBufferSize);
    }

    if (frameDeliminatorAcquired) {
      //#ifdef ARDUINO_ARCH_RP2040
      if (frameSize >= jpegBufferSize) {
        frameSize = 0;
        jpegBufferReadCount = 0;
        frameDeliminatorAcquired = false;
        //cdc.println("ERROR: Received frame size is too big");
        return false;
      }
      *live = true;
      if (frameSize) {
        if (SerialInterface.available() > 0) {
          // If the frame size was determined, get number of bytes to read, check if done filling, then fill if not done
          uint16_t bytesToReadCount = frameSize - (jpegBufferReadCount + 1);


          // Just in case, check if this read will take us out of bounds, if so, restart (shouldn't happen except for future changes forgetting about this)
          if (jpegBufferReadCount + bytesToReadCount < frameSize) {
#ifdef ARDUINO_ARCH_RP2040
            jpegBufferReadCount += cdc.read(jpegBuffer + jpegBufferReadCount, bytesToReadCount);
#else
            while (SerialUSB.available() && bytesToReadCount) {
              jpegBuffer[jpegBufferReadCount] = SerialUSB.read();
              jpegBufferReadCount++;
              bytesToReadCount = frameSize - (jpegBufferReadCount + 1);
            }
            //SerialUSB.println("received some frame data");
#endif
          } else {
            // Not going to get reset by decode so do it here so it doesn't get stuck out of bounds forever
            jpegBufferReadCount = 0;

            frameSize = 0;
            frameDeliminatorAcquired = false;
            cdc.println("ERROR: Tried to place jpeg data out of bounds...");
          }

          if (bytesToReadCount <= 0) {
            //cdc.println("Done receiving frame data");
            //SerialUSB.println("Done receiving frame data");
            frameSize = 0;
            frameDeliminatorAcquired = false;
            *totalBytes = jpegBufferReadCount;
            jpegBufferReadCount = 0;
            return true;
          }
        }
      }
      //#endif
    }
  } else if (millis() - liveTimeoutStart >= liveTimeoutLimitms) {
    // A timeout is a time to reset states of both jpeg buffers, reset everything
    cdc.println("Streaming timeout- outer loop");
    frameSize = 0;
    frameDeliminatorAcquired = false;
    *live = false;
    // Wait for decoding to finish and then reset incoming jpeg data read counts (otherwise may start at something other than zero next time)
  }
  // No buffer filled, wait for more bytes
  return false;
}
