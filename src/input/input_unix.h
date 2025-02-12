#ifndef INPUT_UNIX_H
#define INPUT_UNIX_H

class InputUnix{
    public:
        InputUnix();
        ~InputUnix();

        void poll();
        bool is_power_pressed();
        bool lknob_went_left();
        bool lknob_went_right();
        bool rknob_went_left();
        bool rknob_went_right();
    private:
};

#endif