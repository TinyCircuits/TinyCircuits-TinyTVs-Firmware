#ifndef DISPLAY_ARM_H
#define DISPLAY_ARM_H

#include "display_base.h"
#include <stdint.h>

class DisplayArm : public DisplayBase{
    public:
        DisplayArm(uint16_t width, uint16_t height);
        ~DisplayArm();

        void write(uint16_t *buffer, uint32_t buffer_len) override;
    private:
};

#endif