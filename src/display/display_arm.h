#ifndef DISPLAY_ARM_H
#define DISPLAY_ARM_H

#include <stdint.h>

class DisplayArm{
    public:
        DisplayArm(uint16_t width, uint16_t height);
        ~DisplayArm();

        void write(uint16_t *buffer, uint32_t buffer_len);
        void update();
    private:
        uint16_t width;
        uint16_t height;
};

#endif