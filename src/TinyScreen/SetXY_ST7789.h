void setXST7789(const int32_t x0, const int32_t x1)
{
  stallWait();
  tft_pio->sm[pio_sm].instr = pio_instr_addr;
  TX_FIFO = ST7789_CASET;
  TX_FIFO = ((x0+xOff)<<16) | (x1+xOff);
  TX_FIFO = ST7789_RAMWR;
}

void setYST7789(const int32_t y0, const int32_t y1)
{
  stallWait();
  tft_pio->sm[pio_sm].instr = pio_instr_addr;
  TX_FIFO = ST7789_RASET;
  TX_FIFO = ((y0+yOff)<<16) | (y1+yOff);
  TX_FIFO = ST7789_RAMWR;
}

void setWindowST7789(const int32_t x0, const int32_t y0, const int32_t x1, const int32_t y1)
{
  stallWait();
  tft_pio->sm[pio_sm].instr = pio_instr_addr;
  TX_FIFO = ST7789_CASET;
  TX_FIFO = ((x0+xOff)<<16) | (x1+xOff);
  TX_FIFO = ST7789_RASET;
  TX_FIFO = ((y0+yOff)<<16) | (y1+yOff);
  TX_FIFO = ST7789_RAMWR;
}

void writeCommand(const uint8_t);
void writeData(const uint8_t);

void ST7789InitSeq()
{
  // Regular TinyTV2 init
  writeCommand(ST7789_SLPOUT);
  delay(120);
  writeCommand(ST7789_NORON);
  writeCommand(ST7789_MADCTL);  writeData(TFT_MAD_BGR);
  writeCommand(0xB6); writeData(0x0A); writeData(0x82);
  writeCommand(ST7789_RAMCTRL); writeData(0x00); writeData(0xE0);
  writeCommand(ST7789_COLMOD); writeData(0x55);
  delay(10);
  writeCommand(ST7789_PORCTRL); writeData(0x0c); writeData(0x0c); writeData(0x00); writeData(0x33); writeData(0x33);
  writeCommand(ST7789_GCTRL); writeData(0x35);
  writeCommand(ST7789_VCOMS); writeData(0x28);
  writeCommand(ST7789_LCMCTRL); writeData(0x0C);
  writeCommand(ST7789_VDVVRHEN); writeData(0x01); writeData(0xFF);
  writeCommand(ST7789_VRHS); writeData(0x10);
  writeCommand(ST7789_VDVSET); writeData(0x20);
  writeCommand(ST7789_FRCTR2); writeData(0x0f);
  writeCommand(ST7789_PWCTRL1); writeData(0xa4); writeData(0xa1);

  // Gamma controls
  writeCommand(ST7789_PVGAMCTRL);

  constexpr static uint8_t pv_gamctrl_tab[] =
  {
    0xd0, 0x00, 0x02, 0x07, 0x0a, 0x28, 0x32, 0x44,
    0x42, 0x06, 0x0e, 0x12, 0x14, 0x17,
  };
  for(int i = 0; i < sizeof(pv_gamctrl_tab); i++) writeData(pv_gamctrl_tab[i]);

  writeCommand(ST7789_NVGAMCTRL);

  constexpr static uint8_t nv_gamctrl_tab[] =
  {
    0xd0, 0x00, 0x02, 0x07, 0x0a, 0x28, 0x31, 0x54,
    0x47, 0x0e, 0x1c, 0x17, 0x1b, 0x1e,
  };
  for(int i = 0; i < sizeof(nv_gamctrl_tab); i++) writeData(nv_gamctrl_tab[i]);

  writeCommand(ST7789_INVON);

  writeCommand(ST7789_CASET);    // Column address set
  writeData(0x00);
  writeData(0x00);
  writeData(0x00);
  writeData(0xE5);    // 239

  writeCommand(ST7789_RASET);    // Row address set
  writeData(0x00);
  writeData(0x00);
  writeData(0x01);
  writeData(0x3F);    // 319

  CS_H;
  delay(120);
  CS_L;

  writeCommand(ST7789_DISPON);    //Display on
  delay(120);

  #ifdef TFT_INVERSION_ON
  writeCommand(SSD1357_INVON);
  #endif

  #ifdef TFT_INVERSION_OFF
  writeCommand(SSD1357_INVOFF);
  #endif

  CS_H;
  CS_L;

  writeCommand(ST7789_MADCTL);
  switch (1) {
    case 0: // Portrait
      writeData(TFT_MAD_BGR);
      break;

    case 1: // Landscape (Portrait + 90)
      writeData(TFT_MAD_MX | TFT_MAD_MV | TFT_MAD_BGR);
      break;

      case 2: // Inverter portrait
      writeData(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_RGB);
       break;
    case 3: // Inverted landscape
      writeData(TFT_MAD_MV | TFT_MAD_MY | TFT_MAD_BGR);
      break;
  }
  CS_H;
}
