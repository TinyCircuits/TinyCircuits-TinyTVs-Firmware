// Version scheme: major.minor.patch

// Version increment conditions:
//      * major: incremented when the release contains breaking changes, all other numbers are set to 0
//      * minor: incremented when the release contains new non-breaking features, patch is set to 0
//      * patch: incremented when the release only contains bugfixes and very minor/trivial features considered necessary

// Change this so that the compiled firmware will report its version through:
//  * Serial
//  * The settings files
#define MAJOR 1
#define MINOR 1
#define PATCH 1

// ##### NOTE #####
// IF the firmware version is changed then the following needs to happen to distribute the firmware:
//  1. Either MAJOR, MINOR, or PATCH is changed
//  2. A new binary is exported from the Arduino IDE and placed in the `binaries` folder with its version appended to the end
//  3. Update the latest version signifier in `binaries/update-page-info.json` to the version that file was compiled as (what you put in the name)