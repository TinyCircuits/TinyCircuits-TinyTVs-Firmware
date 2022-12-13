# TinyCircuits-TinyTVs-Firmware
Firmware source for TinyCircuits TinyTV 2 and TinyTV Mini.

Arduino based.

## First time uploading/building
### Getting Arduino board package
1. Follow the instructions here: https://github.com/earlephilhower/arduino-pico#installing-via-arduino-boards-manager
2. Make sure to install version `2.6.0` (newer versions have not been tested)

### Getting the code, configuring IDE, and setting platform
1. `git clone xxx`
2. Extract and copy each folder from the `arduino_libs_to_install` folder to the `Documents/Arduino/libraries` folder (these should not interfere with the `TFT_eSPI` library already installed).
3. Open the XXX.ino file in the Arduino IDE
4. Under `Tools` in the Arduino IDE, set the following (leave others as their defaults)
    * Board: "Raspberry Pi Pico"
    * Flash Size: "2MB (no FS)"
    * CPU Speed: "50MHz" (TinyTV Mini) or "200MHz" (TinyTV 2), NOTE: this is overridden in firmware setup
    * Optimize: "Optimize Even More (-O3)"
    * USB Stack: "Adafruit TinyUSB" <- IMPORTANT!
    * Port: blank on first upload or "COMXXX (Raspberry Pi Pico)" on subsequent uploads
5. Set the `PLATFORM` define to either `TINYTV_2_PLATFORM` or `TINYTV_MINI_PLATFORM` in the `configuration.h` file. This will change the firmware to work for the selected TV/platform

### Changing PIN_LED to work for the volume knob
1. In the Windows taskbar search `run`, execute to bring window up
2. Type `%APPDATA%` and press enter
3. Go up a directory to `Local`
4. Navigate to `Arduino15\packages\rp2040\hardware\rp2040\2.6.0\variants\rpipico\pins_arduino.h`
    1. Change `#define PIN_LED (25u)` -> `#define PIN_LED (12u)`

### Changing TinyUSB USB CDC parameters (increases live frame sending speed)
1. In the Windows taskbar search `run`, execute to bring window up
2. Type `%APPDATA%` and press enter
3. Go up a directory to `Local`
4. Navigate to `Arduino15/packages/rp2040/hardware/rp2040/2.6.0/libraries/Adafruit_TinyUSB_Arduino/src/arduino/ports/rp2040/tusb_config_rp2040.h`
    1. Change `#define CFG_TUD_CDC 1` -> `#define CFG_TUD_CDC 2`
    2. Change `#define CFG_TUD_CDC_RX_BUFSIZE 256` -> `#define CFG_TUD_CDC_RX_BUFSIZE 2048`
    3. Change `#define CFG_TUD_CDC_TX_BUFSIZE 256` -> `#define CFG_TUD_CDC_TX_BUFSIZE 2048`

### Uploading
1. Now that the board package is downloaded, the IDE board upload parameters are set, the IR pin is changed, and the USB CDC parameters are changed, the firmware can be uploaded using the Arduino IDE

## About TFT_eSPI
This library cannot easily be configured to support building firmware for multiple devices on the same machine. The workaround was to make multiple copies, rename each included file, fix include paths in renamed files, and configure the user setups in each copy.

If the library needs updated in the future, make sure to check that the following default files are renamed and reconfigured as was already done in `arduino_libs_to_install`
* `TFT_eSPI.h` -> `TFT_eSPI_tinytvXXX.h` (adjust line `#include <User_Setup_Select_tinytvXXX.h>`)
* `TFT_eSPI.cpp` (adjust line `#include "TFT_eSPI_tinyXXX.h"`)
* `User_Setup_Select.h` -> `User_Setup_Select__tinytvXXX.h (adjust line `#include <User_Setup_tinytvXXX.h>`)`
* `User_Setup.h` -> `User_Setup_tinytvXXX.h` (adjust driver, SPI, and resolution depending on TV)