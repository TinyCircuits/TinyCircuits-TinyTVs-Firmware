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

// SETTINGS DEFAULTS
int channelNumber = 1;
//int volumeSetting = 3;
bool alphabetizedPlaylist = true;
bool loopVideo = true;
bool liveMode = true;
bool doStaticEffects = true;
bool showChannelNumber = true;
bool showVolumeBar = true;
int powerTimeoutSecs = 5 * 60;
bool randStartTime = true;
bool randStartChan = false;
bool roundedCorners = true;

//#define cdc SerialUSB
bool timeStamp = false;

const char keyNames[][15] = {
  "tvType",
  "fwVersion",
  "channel",
  "volume",
  "alphabetize",
  "loopVideo",
  "liveVideo",
  "static",
  "showChannel",
  "showVolume",
#ifndef TinyTVKit
  "powerOffSecs",
#endif
  "randStartTime",
  "randStartChan",
  "roundedCorners",
}; //showTime?

String getKeyValue(String key) {
  if (key == String("tvType")) return String(TYPE_STRING);
  if (key == String("fwVersion")) {
    char version[12];
    sprintf(version, "\"%u.%u.%u\"", MAJOR, MINOR, PATCH);
    return String(version) ;
  }
  if (key == String("channel")) return String(channelNumber);
  if (key == String("volume")) return String(volumeSetting);
  if (key == String("alphabetize")) return String(alphabetizedPlaylist ? "true" : "false");
  if (key == String("loopVideo")) return String(loopVideo ? "true" : "false");
  if (key == String("liveVideo")) return String(liveMode ? "true" : "false");
  if (key == String("static")) return String(doStaticEffects ? "true" : "false");
  if (key == String("showChannel")) return String(showChannelNumber ? "true" : "false");
  if (key == String("showVolume")) return String(showVolumeBar ? "true" : "false");
  if (key == String("powerOffSecs")) return String(powerTimeoutSecs);
  if (key == String("randStartTime")) return String(randStartTime ? "true" : "false");
  if (key == String("randStartChan")) return String(randStartChan ? "true" : "false");
  if (key == String("roundedCorners")) return String(roundedCorners ? "true" : "false");
  return "none";
}

void saveSettings() {
  dbgPrint("saveSettings()");
  File32 settingsFile;
  settingsFile.open("settings.txt", O_WRITE | O_CREAT | O_TRUNC);
  for (int i = 0; i < sizeof(keyNames) / sizeof(keyNames[0]); i++) {
    settingsFile.println(String(keyNames[i]) + "=" + getKeyValue(keyNames[i]));
  }
  settingsFile.close();
}

bool isValidKey(String key) {
  for (int i = 0; i < sizeof(keyNames) / sizeof(keyNames[0]); i++) {
    if (String(keyNames[i]).indexOf(key) >= 0) return true;
  }
  //cdc.println("invalid key!");
  return false;
}

bool setValueByKey(String key, String val) {
  if (key == String("channel")) {
    channelNumber = val.toInt();
    inputFlags.channelSet = true;
  }
  else if (key == String("volume")) {
    volumeSetting = val.toInt();
    inputFlags.volumeSet = true;
  }
  else if (key == String("alphabetize")) alphabetizedPlaylist = (val == (String)"true") ? true : false;
  else if (key == String("loopVideo")) loopVideo = (val == (String)"true") ? true : false;
  else if (key == String("liveVideo")) liveMode = (val == (String)"true") ? true : false;
  else if (key == String("static")) doStaticEffects = (val == (String)"true") ? true : false;
  else if (key == String("showChannel")) showChannelNumber = (val == (String)"true") ? true : false;
  else if (key == String("showVolume")) showVolumeBar = (val == (String)"true") ? true : false;
  else if (key == String("powerOffSecs")) powerTimeoutSecs = max(3,val.toInt());
  else if (key == String("randStartTime")) randStartTime = (val == (String)"true") ? true : false;
  else if (key == String("randStartChan")) randStartChan = (val == (String)"true") ? true : false;
  else if (key == String("roundedCorners")) roundedCorners = (val == (String)"true") ? true : false;
  else return false;
  return true;
}

bool setKeyValue(String line) {
  if ( line.indexOf("=") >= 0) {
    String key = line.substring(0, line.indexOf("="));
    String val = line.substring(line.indexOf("=") + 1);
    key.trim();
    val.trim();
    if ( isValidKey(key)) {
      return setValueByKey(key, val);
    } else {
      // unhandled key - error message?
    }
  }
  return false;
}

void loadSettings() {
  // Load the user settings from settings.txt
  File32 settingsFile;
  if (!settingsFile.open("settings.txt", O_READ)) {
    settingsFile.close();
    saveSettings();
    return;
  } else {
    if (settingsFile.size() == 0) {
      settingsFile.close();
      saveSettings();
      return;
    }
    //settings file with some data in it.
    String line = "";
    while (settingsFile.available()) {
      char c = settingsFile.read();
      if ((c != '\n') && (c != '\r')) {
        line += c;
      }
      if ((c == '\n') || (c == '\r') || (settingsFile.available() == 0)) {
        if (line.length() > 5) {
          //cdc.print(line);
          setKeyValue(line);
        } else {
          //cdc.print("Empty settings file line?: " + line);
        }
        line = "";
      }
    }
    settingsFile.close();
  }
}
