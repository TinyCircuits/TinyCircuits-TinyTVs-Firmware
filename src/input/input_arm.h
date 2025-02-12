#ifndef INPUT_ARM_H
#define INPUT_ARM_H

class InputArm{
    public:
        InputArm();
        ~InputArm();

        void poll();
        bool is_power_pressed();
        bool lknob_went_left();
        bool lknob_went_right();
        bool rknob_went_left();
        bool rknob_went_right();
    private:
};

#endif