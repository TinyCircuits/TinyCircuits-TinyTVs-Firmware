//-------------------------------------------------------------------------------
//  TinyCircuits TinyTV Firmware
//
//  Changelog:
//  10/19/2026 MP4 playback bugfixes, performance improvements, settings-saving behavior changed
//  03/25/2026 MP4 playback update
//  05/26/2023 Initial Release for TinyTV 2/Mini
//  02/08/2023 Cross-platform base committed
//
//  Written by Mason Watmough, Ben Rose, and Jason Marcum for TinyCircuits, http://TinyCircuits.com
//
//-------------------------------------------------------------------------------

volatile int sampleIndex = 0;
volatile int loadedSampleIndex = 0;
volatile int soundVolume = 1;
volatile int audioBufferWrapped = 0;

volatile bool mute = false;

uint32_t currentSampleRate = 0;
uint32_t numberOfStaticSamples = 0;

volatile int pushAudioSample(uint8_t x) {
  if(loadedSampleIndex != sampleIndex) {
    // Data between buf_idx and buf_load
    audioBuf[loadedSampleIndex++] = x;
    if(loadedSampleIndex >= AUDIOBUF_SIZE) {
      loadedSampleIndex = 0;
      audioBufferWrapped = 1;
    }
    return 1;
  } else {
    if(audioBufferWrapped) {
      // Buffer full
      return 0;
    } else {
      audioBuf[loadedSampleIndex++] = x;
      if(loadedSampleIndex >= AUDIOBUF_SIZE) {
        loadedSampleIndex = 0;
        audioBufferWrapped = 1;
      }
      return 1;
    }
  }
}

volatile int dequeueAudioSample() {
  if(loadedSampleIndex != sampleIndex) {
    // Data between buf_idx and buf_load
    sampleIndex++;
    if(sampleIndex >= AUDIOBUF_SIZE) {
      audioBufferWrapped = 0;
      sampleIndex = 0;
    }
    return 1;
  } else {
    if(audioBufferWrapped) {
      sampleIndex++;
      if(sampleIndex >= AUDIOBUF_SIZE) {
        sampleIndex = 0;
        audioBufferWrapped = 0;
      }
      return 1;
    } else {
      // Buffer is empty
      return 0;
    }
  }
}

volatile int peekAudioSample() {
  return audioBuf[sampleIndex];
}

int getAudioSampleCount() {
  if(loadedSampleIndex > sampleIndex) {
    // Data between buf_idx and buf_load
    return loadedSampleIndex - sampleIndex;
  } else {
    // Data from buf_idx to end and then to buf_load
    return (audioBufferWrapped) ? (AUDIOBUF_SIZE-sampleIndex) + loadedSampleIndex : 0;
  }
}

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
//  resetFLACSpillover();
  loadedSampleIndex = sampleIndex;
  audioBufferWrapped = 0;
  memset(audioBuf, 0x80, AUDIOBUF_SIZE);
}

int audioSamplesInBuffer() {
  int samples = loadedSampleIndex - sampleIndex;
  if (samples < 0) {
    samples = AUDIOBUF_SIZE;
  }
  return samples;
}

int addToAudioBuffer(uint8_t * tempBuffer, int len) {
  int ret = 0;
  while ((ret < len) && pushAudioSample(*tempBuffer)) {
    tempBuffer++;
    ret++;
  }
  return ret;
}

#ifndef TinyTVKit
void pwmInterruptHandler(void) {
  int sample = 511;
  sample = peekAudioSample() << 2;
  if(!dequeueAudioSample()) {
    //sample = 0;
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
#ifdef TinyTVMini
    SET_DAC_LEVEL((sample & 0x03FF)>>2);
#else
    SET_DAC_LEVEL(sample & 0x03FF);
#endif
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
  sample = peekAudioSample() << 2;
  dequeueAudioSample();

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
    DAC->DATA.reg =  sample & 0x03FF;
  }

  // Clear the interrupt
  TC5->COUNT16.INTFLAG.bit.MC0 = 1;
}

void TC5_Handler (void) __attribute__ ((weak, alias("Audio_Handler")));

#ifdef __cplusplus
}
#endif
#endif
