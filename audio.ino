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



volatile int sampleIndex = 0;
volatile int loadedSampleIndex = 0;
volatile int soundVolume = 1;

volatile bool mute = false;

uint32_t currentSampleRate = 0;
uint32_t numberOfStaticSamples = 0;

void playStaticFor(uint32_t timeMS) {
  if (currentSampleRate) {
    numberOfStaticSamples = (currentSampleRate * timeMS) / 1000;
  }
}


bool isMute() {
  return mute;
}

void setMute(bool m) {
  mute = m;
}

void setAudioSampleRate(int sr) {
  if (sr != 0) {
    currentSampleRate = sr;
    setAudioHWSampleRate(sr);
  }
}

void setVolume(int vol) {
  noInterrupts();
  soundVolume = vol;
  interrupts();
}

void clearAudioBuffer() {
  sampleIndex = 0;
  loadedSampleIndex = sampleIndex;
  memset(audioBuf, 0x80, AUDIOBUF_SIZE);
}

int audioSamplesInBuffer() {
  int samples = loadedSampleIndex - sampleIndex;
  if (samples < 0) {
    samples += AUDIOBUF_SIZE;
  }
  return samples;
}

void addToAudioBuffer(uint8_t * tempBuffer, int len) {
  while (len) {
    audioBuf[loadedSampleIndex] = *tempBuffer;
    tempBuffer++;
    len--;
    loadedSampleIndex++;
    if ((loadedSampleIndex) >= AUDIOBUF_SIZE) {
      loadedSampleIndex = 0;
    }
  }
}


#ifndef TinyTVKit
void pwmInterruptHandler(void) {
  int sample = 511;
  if (sampleIndex != loadedSampleIndex) {
    sample = audioBuf[sampleIndex] << 2;
    sampleIndex++;
    if (sampleIndex >= AUDIOBUF_SIZE)
      sampleIndex = 0;
  }

  sample -= 511;
  if (soundVolume) {
    sample = sample >> (6 - soundVolume);
  } else {
    sample = 0;
  }
  sample += 511;

  if (numberOfStaticSamples > 0) {
    if (soundVolume) {
      if (sampleIndex == loadedSampleIndex) {
        sample = 511;
      }
      sample += (int)(((rand() & 0xFF) - 128) >> (8 - soundVolume));
    }
    numberOfStaticSamples--;
  }

  if (!mute) {
    SET_DAC_LEVEL(sample & 0x03FF);
  }
  // Clear the interrupt
  CLEAR_DAC_IRQ();
  return;
}
#endif


#ifdef TinyTVKit
#ifdef __cplusplus
extern "C" {
#endif

extern void Audio_Handler (void);
void Audio_Handler (void)
{
  int sample = 511;
  if (sampleIndex != loadedSampleIndex) {
    sample = audioBuf[sampleIndex] << 2;
    sampleIndex++;
    if (sampleIndex >= AUDIOBUF_SIZE)
      sampleIndex = 0;
  }

  sample -= 511;
  if (soundVolume) {
    sample = sample >> (6 - soundVolume);
  } else {
    sample = 0;
  }
  sample += 511;

  if (numberOfStaticSamples > 0) {
    if (soundVolume) {
      if (sampleIndex == loadedSampleIndex) {
        sample = 511;
      }
      sample += (int)(((rand() & 0xFF) - 128) >> (8 - soundVolume));
    }
    numberOfStaticSamples--;
  }

  if (!mute) {
    DAC->DATA.reg =  sample & 0x03FF; ///sample >>= (8 - (soundVolume / 32));
  }

  // Clear the interrupt
  TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}

void TC5_Handler (void) __attribute__ ((weak, alias("Audio_Handler")));

#ifdef __cplusplus
}
#endif
#endif
