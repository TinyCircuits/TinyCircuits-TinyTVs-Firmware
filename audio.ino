//-------------------------------------------------------------------------------
//  TinyCircuits RP2040TV Video Player, Audio Component
//
//  Changelog:
//  08/12/2022 Handed off the keys to the kingdom
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

const uint32_t AUDIOBUF_SIZE = 1024 * 8;
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

void initAudioPin(int pin) {
  audioPinPWMSliceNumber = pwm_gpio_to_slice_num(pin);
  interruptPWMSliceNumber = audioPinPWMSliceNumber + 1;
  gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&config, 1);// This divider controls the base output PWM frequency
  pwm_config_set_wrap(&config, 255);    // 8 bit audio output
  pwm_init(audioPinPWMSliceNumber, &config, true);
}

void setAudioSampleRate(int sr) {
  //generate the interrupt at the audio sample rate to set the PWM duty cycle
  pwm_clear_irq(interruptPWMSliceNumber);
  pwm_set_irq_enabled(interruptPWMSliceNumber, true);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, pwmInterruptHandler);
  irq_set_enabled(PWM_IRQ_WRAP, true);
  pwm_config configInt = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&configInt, 1);
  pwm_config_set_wrap(&configInt, (CPU_HZ / sr) - 1);
  pwm_init(interruptPWMSliceNumber, &configInt, true);
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
  //Serial.println(audioSamplesInBuffer());
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
    int sample = (int)(((audioBuf[(sampleIndex)]) * soundVolume) >> 8);
    if (playWhiteNoise) {
      sample += (int)((((rand() & 0xFF) - 128) * soundVolume) >> 12);
    }
    if (!mute) {
      pwm_set_gpio_level(AUDIO_PIN, uint16_t(sample));
    }
    if ((sampleIndex != loadedSampleIndex))
      sampleIndex++;
    if ((sampleIndex) >= AUDIOBUF_SIZE)
      sampleIndex -= AUDIOBUF_SIZE;
  }
  // Clear the interrupt
  pwm_clear_irq(interruptPWMSliceNumber);
  return;
}
