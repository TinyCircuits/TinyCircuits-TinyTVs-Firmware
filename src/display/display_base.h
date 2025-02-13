#ifndef DISPLAY_BASE_H
#define DISPLAY_BASE_H

#include <stdint.h>

// Platform dependent classes inherit this class
// that defines the format and common logic
class DisplayBase{
    public:
        DisplayBase(uint16_t width, uint16_t height);
        ~DisplayBase();

        // Writes `buffer` that's `buffer_len` RGB565 pixels long to screen
        virtual void write(uint16_t *buffer, uint32_t buffer_len) = 0;  // `= 0`, pure, must be implemented by derived class
    private:
        uint16_t width;     // Display width (px)
        uint16_t height;    // Display height (px)
};

#endif