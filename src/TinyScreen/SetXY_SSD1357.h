void setXSSD1357(const int32_t x0, const int32_t x1)
{
  stallWait();
  tft_pio->sm[pio_sm].instr = pio_instr_addr;
  clearDC(); write8(SSD1357_PASET);
  setDC(); write16((x1+xOff) | ((x0+xOff) << 8));
  clearDC(); write8(SSD1357_RAMWR);
  setDC();
}

void setYSSD1357(const int32_t y0, const int32_t y1)
{
  stallWait();
  tft_pio->sm[pio_sm].instr = pio_instr_addr;
  clearDC(); write8(SSD1357_CASET);
  setDC(); write16((y1+yOff) | ((y0+yOff) << 8));
  delayMicroseconds(10);
  clearDC();
  delayMicroseconds(10);
  write8(SSD1357_RAMWR);
  delayMicroseconds(10);
  setDC();
}

void setWindowSSD1357(const int32_t y0, const int32_t x0, const int32_t y1, const int32_t x1)
{
  stallWait();
  tft_pio->sm[pio_sm].instr = pio_instr_addr;
  clearDC(); write8(colSetCommand); // DC clear
  setDC(); write16((x1+yOff) | ((x0+yOff) << 8)); // DC drive
  clearDC(); write8(rowSetCommand);
  setDC(); write16(y1 | (y0 << 8));
  clearDC(); write8(writeRamCommand);
  setDC();
}

void writeCommand(const uint8_t);
void writeData(const uint8_t);

void SSD1357InitSeq()
{
  // RP2040TVMini init
  writeCommand(0xFD); writeData(0x12);
  writeCommand(0xAE); // DISPLAYOFF
  writeCommand(0xB3); writeData(0x20); //missing/typo-> mfg says 0xB0, but seems like default 0x20 is better
  writeCommand(0xCA); writeData(0x3F); //was 0x7F, should be 0x3F
  writeCommand(0xA2); writeData(0x40);
  writeCommand(0xB1); writeData(0x32);
  writeCommand(0xBE); writeData(0x05);
  writeCommand(0xB6); writeData(0x01);
  writeCommand(0xBB); writeData(0x17);
  writeCommand(0xA6); // NORMALDISPLAY
  writeCommand(0xC1); writeData(0x88+4); writeData(0x32); writeData(0x88-4); //adjust red down slightly
  writeCommand(0xC7); writeData(0x0F);
  int j;
  constexpr static uint8_t gamma[63]=
  {
    0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,
    0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x15,0x17,
    0x19,0x1B,0x1D,0x1F,0x21,0x23,0x25,0x27,0x2A,0x2D,
    0x30,0x33,0x36,0x39,0x3C,0x3F,0x42,0x45,0x48,0x4C,
    0x50,0x54,0x58,0x5C,0x60,0x64,0x68,0x6C,0x70,0x74,
    0x78,0x7D,0x82,0x87,0x8C,0x91,0x96,0x9B,0xA0,0xA5,
    0xAA,0xAF,0xB4,
  };
  writeCommand(0xB8);			// Set Gray Scale Table

  for(j=0;j<63;j++)
  {
      writeData(gamma[j]);
  }
  writeCommand(0xAF); // DISPLAYON

  #ifdef TFT_INVERSION_ON
  writeCommand(SSD1357_INVON);
  #endif

  #ifdef TFT_INVERSION_OFF
  writeCommand(SSD1357_INVOFF);
  #endif

  CS_H;

  // Set rotation
  CS_L;

  uint8_t madctl = 0x64;

  switch (1) {
    case 0:
      madctl |= 0x12;
      break;
    case 1:
      madctl |= 0x11;
      break;
    case 2:
      madctl |= 0x00;
      break;
    case 3:
      madctl |= 0x03;
      break;
  }

  writeCommand(0xA0);
  writeData(madctl);
  writeData(0x00);
  delay(10);
  CS_H;
}
