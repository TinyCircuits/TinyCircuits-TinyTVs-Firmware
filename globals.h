//-------------------------------------------------------------------------------
//  TinyCircuits TinyTV Firmware
//
//  Changelog:
//  10/19/2026 MP4 playback bugfixes, performance improvements, settings-saving behavior changed
//  03/25/2026 MP4 playback update
//  05/26/2023 Initial Release for TinyTV 2/Mini
//  02/08/2023 Cross-platform base committed
//
//  Written by Mason Watmough, Ben Rose, and Jason Marcum for TinyCircuits, http://TinyCircuits.com
//
//-------------------------------------------------------------------------------

typedef struct inputFlagStruct{
  bool channelUp = false;
  bool channelDown = false;
  bool volUp = false;
  bool volDown = false;
  bool mute = false;
  bool power = false;
  bool channelSet = false;
  bool volumeSet = false;
  bool settingsChanged = false;
}inputFlagStruct;


// PLAYBACK PARAMETERS
uint64_t targetFrameTime;


int volumeSetting = 3;

inputFlagStruct inputFlags;
