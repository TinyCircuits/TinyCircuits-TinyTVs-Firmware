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


volatile struct TinyIRReceiverCallbackDataStruct sCallbackData;

void initializeInfrared() {
  if (!initPCIInterruptForTinyReceiver()) {
    dbgPrint("No interrupt available for IR_INPUT_PIN");
  }
}

void handleReceivedTinyIRData(uint16_t aAddress, uint8_t aCommand, bool isRepeat){
  sCallbackData.Address = aAddress;
  sCallbackData.Command = aCommand;
  sCallbackData.isRepeat = isRepeat;
  sCallbackData.justWritten = true;
}

void IRInput(inputFlagStruct * inputFlags){
  if (sCallbackData.justWritten){
    sCallbackData.justWritten = false;
    if (sCallbackData.Command == 0x1)
      inputFlags->channelUp = true;
    if (sCallbackData.Command == 0x4)
      inputFlags->channelDown = true;
    if (sCallbackData.Command == 0xF)
      inputFlags->mute = true;
    if (sCallbackData.Command == 0xD)
      inputFlags->volDown = true;
    if (sCallbackData.Command == 0xE)
      inputFlags->volUp = true;
    if (sCallbackData.Command == 0x11)
      inputFlags->power = true;
  }
}
