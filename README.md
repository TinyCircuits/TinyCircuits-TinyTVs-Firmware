# TinyCircuits-TinyTVs-Firmware

This repository contains the firmware source for JPEG AVI playback (and serial JPEG streaming for TinyTV 2 and Mini) on the following TinyCircuits platforms:
* TinyTV 2
* TinyTV Mini
* TinyTV DIY

See the TinyTV-Site repo for even more information and utility pages for streaming, updating, and changing settings: https://github.com/TinyCircuits/TinyTV-site

## Disclaimer about contributions

Although we greatly appreciate feedback and contributions to our product's code, as a business it takes time to integrate, test, and deploy new versions of firmware. As is typical with software, there is an inherent risk of additional bugs and breakages upon release of new firmware, therefore, only contributions to this codebase that greatly improve the product and its functionally can be considered.

## Versions/Changing the code

Two steps **need** to be done to release a new version of the firmware:
1. In `versions.h` update the firmware version (format `major.minor.patch`) in the defines according to these rules:
    * `major`: incremented when the release contains breaking changes, all other numbers are set to 0
    * `minor`: incremented when the release contains new non-breaking features, patch is set to 0
    * `patch`: incremented when the release only contains bugfixes and very minor/trivial features considered necessary

Read the rest of the `versions.h` file for updating the `REQUIRED` versions. The first version ever was `1.0.0`

2. New binaries with the following names need to be committed at the same time with a commit message noting the version they were compiled:
    * `binaries/TinyTV-2-firmware.uf2`
    * `binaries/TinyTV-Mini-firmware.uf2`
    * `binaries/TinyTV-DIY-firmware.bin`

The names need to remain the same for the `TinyTV-Site` update page to find them. Make sure the UF2s are placed in this repository's `binaries` folder.

## Compiling: Arduino IDE

Follow the below steps to build a firmware image using the Arduino IDE.

### 1. Installing libraries

Unzip and move each of the folders in `src` (called that instead of 'lib' because of Arduino nomenclature) to your `Documents/Arduino/libraries` folder
* `JPEGDEC`: library for decoding JPEG frames for serial streaming and JPEG AVI video playback
* `SdFat`: library for interacting with SD cards
* `IRremote`: for handling the remote IR receiver and IR codes

Leave these custom libraries in the `src` folder
* `TinyScreen`: modified TinyScreen+ library to include support for TFT displays
* `GraphicsBuffer2`: modified TinyScreen+ library for text rendering in an arbitrary buffer location

### 2. Install the RP2040 Arduino board package (TinyTV 2 and TinyTV Mini)

Follow the steps listed here: https://github.com/earlephilhower/arduino-pico#installing-via-arduino-boards-manager. Install version 2.6.0.

### 3. Install the TinyCircuits Arduino board package (TinyTV DIY Kit)
Follow the steps here https://learn.tinycircuits.com/Processors/TinyScreen%2B_Setup_Tutorial/ under "Step 1: Configure Arduino Software IDE"

### 4. Compiling for each platform (TinyTV 2, TinyTV Mini, or TinyTV DIY Kit)

**TinyTV 2 and Mini**

1. In the Arduino IDE under `Tools`, make sure to set each of the following:
    * Board: Raspberry Pi Pico
    * CPU Speed: 200MHz for TinyTV 2 and 50MHz for TinyTV Mini
    * Optimization: -Os
    * USB Stack: Adafruit TinyUSB

2. Change the following in the RP2040 board package
    1. In the Windows taskbar search `run`, execute to bring window up
    2. Type `%APPDATA%` and press enter
    3. Go up a directory to `Local`
        1. Navigate to `Arduino15\packages\rp2040\hardware\rp2040\2.6.0\variants\rpipico\pins_arduino.h`
        2. Change `#define PIN_LED (25u)` -> `#define PIN_LED (12u)`
    4. Go back to `Local`
        1. Navigate to `Arduino15/packages/rp2040/hardware/rp2040/2.6.0/libraries/Adafruit_TinyUSB_Arduino/src/arduino/ports/rp2040/tusb_config_rp2040.h`
        2. Change `#define CFG_TUD_CDC 1` -> `#define CFG_TUD_CDC 2` (reason: to use Adafruit's CDC instead of Arduino Serial)
        3. Change `#define CFG_TUD_CDC_RX_BUFSIZE 256` -> `#define CFG_TUD_CDC_RX_BUFSIZE 2048` (reason: faster serial, hand tweaked to a fast working value)
        4. Change `#define CFG_TUD_CDC_TX_BUFSIZE 256` -> `#define CFG_TUD_CDC_TX_BUFSIZE 2048` (reason: faster serial, hand tweaked to a fast working value)
        5. Make sure to set `#define CFG_TUD_MSC_EP_BUFSIZE 512*8` (reason: faster USB Mass Storage transfer rates) 

**TinyTV DIY Kit**

1. In the Arduino IDE under `Tools`, make sure to set the following:
    * Board: TinyScreen+

**TinyTV 2, TinyTV Mini, or TinyTV DIY Kit**

Uncomment one of these lines in the main .ino file to select the hardware target:
//#include "TinyTV2.h"
//#include "TinyTVMini.h"
//#include "TinyTVKit.h"

Now you can press the upload button (after choosing a port and/or putting the device into bootloader mode) or use `Sketch -> Export compiled Binary`.
