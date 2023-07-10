//-------------------------------------------------------------------------------
//  TinyCircuits TinyTV Firmware
//
//  Changelog:
//  05/26/2023 Initial Release for TinyTV 2/Mini
//  02/08/2023 Cross-platform base committed
//
//  Written by Mason Watmough, Ben Rose, and Jason Marcum for TinyCircuits, http://TinyCircuits.com
//
//-------------------------------------------------------------------------------


volatile bool frameReady[2] = {false, false};
volatile bool frameDecoded[2] = {true, true};
volatile int decoderDataLength[2] = {0, 0};
volatile uint8_t currentWriteBuf = 0;
volatile uint8_t currentDecodeBuf = 0;
volatile uint32_t lastBufferAssignment = 0;


void JPEGBufferFilled(int length) {
  decoderDataLength[currentWriteBuf] = length;
  frameDecoded[currentWriteBuf] = false;
  frameReady[currentWriteBuf] = true;
  if (DOUBLE_BUFFER)
    currentWriteBuf = 1 - currentWriteBuf;
}

uint8_t * getFilledJPEGBuffer() {
  uint8_t filledBuffer = currentWriteBuf;
  if (DOUBLE_BUFFER)
    filledBuffer = 1 - currentWriteBuf;
  if (frameReady[filledBuffer]) {
    currentDecodeBuf = filledBuffer;
    return videoBuf[filledBuffer];
  }
  return NULL;
}

int getJPEGBufferLength() {
  return decoderDataLength[currentDecodeBuf];
}

void resetBuffers() {
  frameReady[0] = false; frameReady[1] = false;
  frameDecoded[0] = true; frameDecoded[1] = true;
  dbgPrint("resetBuffers");
}

uint8_t * getFreeJPEGBuffer() {
  if (millis() - lastBufferAssignment > 250) {
    resetBuffers();
  }
  if (frameDecoded[currentWriteBuf] == true && frameReady[currentWriteBuf] == false) {
    lastBufferAssignment = millis();
    return videoBuf[currentWriteBuf];
  }
  return NULL;
}

void JPEGBufferDecoded() {
  decoderDataLength[currentDecodeBuf] = 0;
  frameReady[currentDecodeBuf] = false;
  frameDecoded[currentDecodeBuf] = true;
}
