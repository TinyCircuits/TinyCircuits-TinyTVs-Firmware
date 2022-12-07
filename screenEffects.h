#ifndef SCREEN_EFFECTS_H
#define SCREEN_EFFECTS_H

#include <stdint.h>
#include <stdlib.h>
#include <cmath>


class RoundCornerEffect{
  public:
    RoundCornerEffect(uint8_t tinyTVType);
    void cropCorners(uint16_t *screenBuffer, uint8_t width, uint8_t height);

  private:
    void calculateLimits();

    // Array of x radius limits allocated and calculated on class initialization
    uint8_t *cropRadiusLimits;

    // Radius in pixels
    const uint8_t tinyTV2CropRadius = 25;
    const uint8_t tinyTVMiniCropRadius = 8;
    uint8_t cropRadius = 0;
};


class StaticEffects: public RoundCornerEffect{
  public:
    StaticEffects(uint8_t tinyTVType);

    // Start the effect for when the channel is changed (overrides the effect if active)
    void startChangeChannelEffect();

    // Start the effect for when the TV turns off (overrides the effect if active)
    void startTurnOffEffect();

    // Process either the 'change channel' or 'turn off' effects
    bool processStartedEffects(uint16_t *screenBuffer, uint8_t width, uint8_t height);
  private:
    enum{
      NONE=0,
      CHANGE_CHANNEL,
      TURN_OFF,
    };

    void makeStaticEffectFrame(uint16_t *screenBuffer, uint8_t width, uint8_t height);
    void processChangeChannelEffect(uint16_t *screenBuffer, uint8_t width, uint8_t height);
    void processTurnOffEffect(uint16_t *screenBuffer, uint8_t width, uint8_t height);

    uint8_t currentStartedEffect = StaticEffects::NONE;

    uint8_t changeChannelFrameIndex = 0;
    uint8_t changeChannelFrameCount = 3;

    uint8_t turnOffFrameIndex = 0;
};


// Apply various screen effects to screen buffer like static or round corner cropping
class ScreenEffects : public StaticEffects{
  public:
    ScreenEffects(uint8_t tinyTVType);

    enum TINYTV_TYPE{
      TINYTV_2=0,
      TINYTV_MINI
    };

  private:
};


#endif