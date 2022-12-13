#ifndef SCREEN_EFFECTS_H
#define SCREEN_EFFECTS_H

#include <stdint.h>
#include <stdlib.h>
#include <cmath>
#include <cstring>
#include <Arduino.h>


class RoundCornerEffect{
  public:
    RoundCornerEffect(uint8_t width, uint8_t height);

    void setCropRadius(uint8_t radius);
    void cropCorners(uint16_t *screenBuffer, uint8_t width, uint8_t height);

    // Default to minimum (works well on mini)
    uint8_t cropRadius = 5;
    uint8_t lastCropRadius = cropRadius;
    
    uint8_t screenWidth;
    uint8_t screenHeight;
  private:
    void calculateLimits();

    // Array of x radius limits allocated and calculated on class initialization
    uint8_t *cropRadiusLimits;
    uint8_t cropRadiusLimitsSize;
};


class StaticEffects: public RoundCornerEffect{
  public:
    StaticEffects(uint8_t width, uint8_t height);

    // Start the effect for when the channel is changed (overrides the effect if active)
    void startChangeChannelEffect();

    // Start the effect for when the TV turns off (overrides the effect if active)
    void startTurnOffEffect();

    void stopEffects();

    // Process either the 'change channel' or 'turn off' effects
    bool processStartedEffects(uint16_t *screenBuffer, uint8_t width, uint8_t height);
  private:
    enum{
      NONE = 0,
      CHANGE_CHANNEL,
      TURN_OFF,
    };

    void makeStaticEffectFrame(uint16_t *screenBuffer, uint8_t width, uint8_t height);
    void processChangeChannelEffect(uint16_t *screenBuffer, uint8_t width, uint8_t height);
    void processTurnOffEffect(uint16_t *screenBuffer, const uint8_t width, const uint8_t height);

    uint8_t currentStartedEffect = StaticEffects::NONE;

    uint8_t changeChannelFrameIndex = 0;
    uint8_t changeChannelFrameCount = 3;

    uint8_t turnOffRadius = 120;
    uint8_t *turnOffRadiusLimits;
};


// Apply various screen effects to screen buffer like static or round corner cropping
class ScreenEffects : public StaticEffects{
  public:
    ScreenEffects(uint8_t width, uint8_t height);
  private:
};


#endif