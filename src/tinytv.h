#ifndef TINYTV_H
#define TINYTV_H

#include <stdint.h>
#include "files/files.h"
#include "display/display.h"

enum class TVErrorCode : uint8_t{
    TV_OK=0
};

class TinyTV{
    public:
        TinyTV(Files *files, Display *display);
        ~TinyTV();

        TVErrorCode processVideo();
    private:
        Files *files;
        Display *display;
};

#endif