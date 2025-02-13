#ifndef INPUT_BASE_H
#define INPUT_BASE_H


#include <stdint.h>


#define INPUT_FUNC_CODE_PWR         0b0000000000000001
#define INPUT_FUNC_CODE_NEXT_CHAN   0b0000000000000010
#define INPUT_FUNC_CODE_PREV_CHAN   0b0000000000000100
#define INPUT_FUNC_CODE_VOL_UP      0b0000000000001000
#define INPUT_FUNC_CODE_VOL_DOWN    0b0000000000010000


// Unique pins for TV2
typedef struct tv2_input_t{
    uint8_t lencoder_pin_a;
    uint8_t lencoder_pin_b;
    uint8_t rencoder_pin_a;
    uint8_t rencoder_pin_b;
}tv2_pins_t;

// Unique pins for TV Mini
typedef struct tvmini_input_t{
    uint8_t lbtn_pin;
    uint8_t rbtn_pin;
}tvmini_pins_t;

// Unique pins for TV Kit
typedef struct tvkit_input_t{
    uint8_t lside_top_btn_pin;
    uint8_t lside_bot_btn_pin;
    uint8_t rside_top_btn_pin;
    uint8_t rside_bot_btn_pin;
}tvkit_pins_t;

// Consolidated structure of all pins
typedef struct input_t{
    uint8_t tv_type;
    
    uint8_t pwr_btn_pin;
    uint8_t ir_pin;

    union{
        struct tv2_input_t    tv2_pins;
        struct tvmini_input_t tvmini_pins;
        struct tvkit_input_t  tvkit_pins;
    }unique;
}tv_pins_t;


// Platform dependent classes inherit this class
// that defines the format and common logic
class InputBase{
    public:
        InputBase(tv_pins_t *pins);
        ~InputBase();

        // Call this as fast as possible to poll
        // and store button states (implemented
        // on some platforms but done by
        // interrupts on others - arm)
        virtual void poll() = 0;    // `= 0`, pure, must be implemented by derived class

        // Each of these return true when the respective TV
        // function is active. This can be due to a physical
        // button press or due to IR codes from a remote
        bool is_power_pressed(bool check_just_pressed=true);
        bool is_next_channel_pressed(bool check_just_pressed=true);
        bool is_prev_channel_pressed(bool check_just_pressed=true);
        bool is_vol_up_pressed(bool check_just_pressed=true);
        bool is_vol_down_pressed(bool check_just_pressed=true);
    private:
    protected:
        uint16_t last_pressed;  // Last TV functions pressed (channel, volume, power, etc.)
        uint16_t pressed;       // TV functions pressed (channel, volume, power, etc.)
        tv_pins_t *pins;        // Pins to use (contains TV type)
};

#endif