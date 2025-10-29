// TinyCircuits will handle updating the version number when creating a new firmware release. When we distribute the new firmware, we will:
//  1. Either MAJOR, MINOR, or PATCH is changed
//  2. A new binary is exported from the Arduino IDE and placed in the `binaries` folder with its version appended to the end
//  3. Update the latest version signifier in `binaries/update-page-info.json` to the version that file was compiled
//  4. Add information to releaseChangelog.txt
//  5. Delete old binaries

// Version scheme: major.minor.patch

// Version increment conditions:
//      * major: incremented when the release contains breaking changes, all other numbers are set to 0
//      * minor: incremented when the release contains new non-breaking features, patch is set to 0
//      * patch: incremented when the release only contains bugfixes and very minor/trivial features considered necessary

// Change this so that the compiled firmware will report its version through:
//  * Serial
//  * The settings files
#define MAJOR 1
#define MINOR 2
#define PATCH 6
