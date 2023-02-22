// Version scheme: major.minor.patch

// Version increment conditions:
//      * major: incremented when the release contains breaking changes, all other numbers are set to 0
//      * minor: incremented when the release contains new non-breaking features, patch is set to 0
//      * patch: incremented when the release only contains bugfixes and very minor/trivial features considered necessary

// The intention is to have a single 'OS' version number so as not to confuse customers with an alterantive of multiple
// If this is ever incremented, every platform's binary needs to be recomplied to ensure the latest version below is reported
// even if the change did not affect every platform.
#define MAJOR 1
#define MINOR 0
#define PATCH 0

// *** These comments are used by the update page, update them when the above MAJOR, MINOR, or PATCH numbers change ***
// If a change is made that doesn't affect a specific platform, thses can be used to NOT notify the user of an update
// on the update webpage.
// CONDITIONS:
//  * Increase and match the version number to the above defines if a change is made in the code that will affect a specific platform
//  * If the code change did not affect a specific platform, do not increase/match the version number to the above defines for that platform
//  * Example: made change that affects TV2 and Mini but not DIY. Increase the above defines according to the increment rules at the top of this file
//             to 1.0.1. TV2 and Mini need this new version and users should be notified, increase the below required versions to 1.0.1 for each but
//             not for the DIY kit, it stays at 1.0.0 since users shouldn't be notifed as it does not affect their TV. The website will look at the
//             the firmware version reported by the connected TV and check if it is below its required verson below. If below, the website will fetch
//             the latest firmware file from GitHub and upload it. Even if the required version below doesn't change for a platform, a new firmware binary
//             needs to compiled so as to report the latest MAJOR, MINOR, and PATCH version
// TINYTV_2_VERSION_REQUIRED_MAJOR 1
// TINYTV_2_VERSION_REQUIRED_MINOR 0
// TINYTV_2_VERSION_REQUIRED_PATCH 0

// TINYTV_MINI_VERSION_REQUIRED_MAJOR 1
// TINYTV_MINI_VERSION_REQUIRED_MINOR 0
// TINYTV_MINI_VERSION_REQUIRED_PATCH 0

// TINYTV_DIY_VERSION_REQUIRED_MAJOR 1
// TINYTV_DIY_VERSION_REQUIRED_MINOR 0
// TINYTV_DIY_VERSION_REQUIRED_PATCH 0