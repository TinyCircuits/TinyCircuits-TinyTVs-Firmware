//-------------------------------------------------------------------------------
//  TinyCircuits RP2040TV Video Player, Audio Component
//
//  Changelog:
//  08/12/2022 Handed off the keys to the kingdom
//  
//  02/08/2023 Cross-platform base committed
//
//  Written by Mason Watmough for TinyCircuits, http://TinyCircuits.com
//  Heavily adapted from software originally written by Ben Rose
//-------------------------------------------------------------------------------

/*
    This file is part of the RP2040TV Player.
    RP2040TV Player is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    RP2040TV Player is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with
    the RP2040TV Player. If not, see <https://www.gnu.org/licenses/>.
*/

const uint32_t AUDIOBUF_SIZE = 1024 * 2;
uint8_t audioBuf[AUDIOBUF_SIZE] = {127};

volatile int sampleIndex = 0;
volatile int loadedSampleIndex = 0;

int audioPinPWMSliceNumber = 0;
int interruptPWMSliceNumber = 0;


volatile bool mute = false;

bool isMute() {
  return mute;
}

void setMute(bool m) {
  mute = m;
}
bool tcIsSyncing()
{
  #ifdef TinyTVKit
  return TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY;
  #else
  return false;
  #endif
}

void initAudioPin(int pin) {
#ifndef TinyTVKit
  digitalWrite(SPK_EN, LOW); // Speaker disable
  audioPinPWMSliceNumber = pwm_gpio_to_slice_num(pin);
  interruptPWMSliceNumber = audioPinPWMSliceNumber + 1;
  gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&config, 1);// This divider controls the base output PWM frequency
  pwm_config_set_wrap(&config, 1023);    // 8 bit audio output
  pwm_init(audioPinPWMSliceNumber, &config, true);
  if (!mute) digitalWrite(23, HIGH); // Speaker enable
#else
  analogWrite(A0, analogRead(A0));
#endif
}

void setAudioSampleRate(int sr) {
#ifndef TinyTVKit
  //generate the interrupt at the audio sample rate to set the PWM duty cycle
  pwm_clear_irq(interruptPWMSliceNumber);
  pwm_set_irq_enabled(interruptPWMSliceNumber, true);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, pwmInterruptHandler);
  irq_set_enabled(PWM_IRQ_WRAP, true);
  pwm_config configInt = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&configInt, 1);
  pwm_config_set_wrap(&configInt, (CPU_HZ / sr) - 1);
  pwm_init(interruptPWMSliceNumber, &configInt, true);
#else

  // Enable GCLK for TCC2 and TC5 (timer counter input clock)
  GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
  while (GCLK->STATUS.bit.SYNCBUSY);

  
  // Reset TCx
  TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (tcIsSyncing());
  while (TC5->COUNT16.CTRLA.bit.SWRST);

  // Set Timer counter Mode to 16 bits
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;

  // Set TC5 mode as match frequency
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;

  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1;

  TC5->COUNT16.CC[0].reg = (uint16_t) (SystemCoreClock / sr - 1);
  while (tcIsSyncing());

  // Configure interrupt request
  NVIC_DisableIRQ(TC5_IRQn);
  NVIC_ClearPendingIRQ(TC5_IRQn);
  NVIC_SetPriority(TC5_IRQn, 0);
  NVIC_EnableIRQ(TC5_IRQn);

  // Enable the TC5 interrupt request
  TC5->COUNT16.INTENSET.bit.MC0 = 1;
  while (tcIsSyncing());
  // Enable TC

  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
  while (tcIsSyncing());
#endif
}

void clearAudioBuffer() {
  sampleIndex = 0;
  loadedSampleIndex = sampleIndex;
}

int audioSamplesInBuffer() {
  int samples = loadedSampleIndex - sampleIndex;
  if (samples < 0) {
    samples += AUDIOBUF_SIZE;
  }
  return samples;
}

void addToAudioBuffer(uint8_t * tempBuffer, int len) {
  if (len < AUDIOBUF_SIZE) {
    int readLen = 0;
    noInterrupts();
    while (len - readLen > 0) {
      int numBytes = min(AUDIOBUF_SIZE - loadedSampleIndex, len - readLen);
      memcpy((uint8_t*)audioBuf + (loadedSampleIndex), tempBuffer, numBytes);
      readLen += numBytes;
      loadedSampleIndex += numBytes;
      if ((loadedSampleIndex) >= AUDIOBUF_SIZE)
      {
        loadedSampleIndex -= AUDIOBUF_SIZE;
      }
    }
    interrupts();
  }
}

void pwmInterruptHandler(void) {
  if ((sampleIndex != loadedSampleIndex) || playWhiteNoise) {
    int sample = (int)(((audioBuf[(sampleIndex)]) * soundVolume) >> 6);
    if (doStaticEffects && staticTimer > 0) {
      sample += (int)((((rand() & 0xFF) - 128) * soundVolume) >> 9);
    }
    if (!mute) {
#ifndef TinyTVKit
      pwm_set_gpio_level(AUDIO_PIN, uint16_t(sample));
#else
#endif
    }
    if ((sampleIndex != loadedSampleIndex))
      sampleIndex++;
    if ((sampleIndex) >= AUDIOBUF_SIZE)
      sampleIndex -= AUDIOBUF_SIZE;
  }
  // Clear the interrupt
#ifndef TinyTVKit
  pwm_clear_irq(interruptPWMSliceNumber);
#else
#endif
  return;
}
#ifdef TinyTVKit
#ifdef __cplusplus
extern "C" {
#endif

extern void Audio_Handler (void);
int dacOut;
void Audio_Handler (void)
{
  if ((sampleIndex != loadedSampleIndex) || playWhiteNoise) {
    uint16_t sample = audioBuf[sampleIndex];
    if (doStaticEffects && staticTimer > 0) {
      sample += (int)((((rand() & 0xFF) - 128) * soundVolume) >> 9);
    }
    if (!mute) {
      while (DAC->STATUS.bit.SYNCBUSY == 1);
      sample <<= (2);
      DAC->DATA.reg =  sample >>= (8-(soundVolume/32));
    }
    
    if ((sampleIndex != loadedSampleIndex))
      sampleIndex++;
    if ((sampleIndex) >= AUDIOBUF_SIZE)
      sampleIndex -= AUDIOBUF_SIZE;
  }

  // Clear the interrupt
  TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}

void TC5_Handler (void) __attribute__ ((weak, alias("Audio_Handler")));

#ifdef __cplusplus
}
#endif
#endif
