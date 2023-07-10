void setXGC9A01(const int32_t x0, const int32_t x1)
{
  stallWait();
  tft_pio->sm[pio_sm].instr = pio_instr_addr;
  TX_FIFO = GC9A01_CASET;
  TX_FIFO = ((x0+xOff)<<16) | (x1+xOff);
  TX_FIFO = GC9A01_RAMWR;
}

void setYGC9A01(const int32_t y0, const int32_t y1)
{
  stallWait();
  tft_pio->sm[pio_sm].instr = pio_instr_addr;
  TX_FIFO = GC9A01_PASET;
  TX_FIFO = ((y0+yOff)<<16) | (y1+yOff);
  TX_FIFO = GC9A01_RAMWR;
}

void setWindowGC9A01(const int32_t x0, const int32_t y0, const int32_t x1, const int32_t y1)
{
  stallWait();
  tft_pio->sm[pio_sm].instr = pio_instr_addr;
  TX_FIFO = GC9A01_CASET;
  TX_FIFO = ((x0+xOff)<<16) | (x1+xOff);
  TX_FIFO = GC9A01_PASET;
  TX_FIFO = ((y0+yOff)<<16) | (y1+yOff);
  TX_FIFO = GC9A01_RAMWR;
}

void writeCommand(const uint8_t);
void writeData(const uint8_t);

void GC9A01InitSeq()
{
  writeCommand(0xEF);
  writeCommand(0xEB);
  writeData(0x14);
  writeCommand(0xFE);
  writeCommand(0xEF);

  constexpr static uint8_t init_com_tab[] =
  {
    0xeb, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  };
  constexpr static uint8_t init_arg_tab[] =
  {
    0x14, 0x40, 0xff, 0xff, 0xff, 0x0a, 0x21, 0x00, 0x80, 0x01, 0x01, 0xff, 0xff
  };
  for(int i = 0; i < sizeof(init_com_tab); i++) { writeCommand(init_com_tab[i]); writeData(init_arg_tab[i]); }

  writeCommand(0xB6); writeData(0x00); writeData(0x20);
  writeCommand(0x3A); writeData(0x05);
  writeCommand(0x90); writeData(0x08); writeData(0x08); writeData(0x08); writeData(0x08);
  writeCommand(0xBD); writeData(0x06);
  writeCommand(0xBC); writeData(0x00);
  writeCommand(0xFF); writeData(0x60); writeData(0x01); writeData(0x04);
  writeCommand(0xC3); writeData(0x13);
  writeCommand(0xC4); writeData(0x13);
  writeCommand(0xC9); writeData(0x22);
  writeCommand(0xBE); writeData(0x11);
  writeCommand(0xE1); writeData(0x10); writeData(0x0E);
  writeCommand(0xDF); writeData(0x21); writeData(0x0c); writeData(0x02);
  writeCommand(0xF0); writeData(0x45); writeData(0x09); writeData(0x08); writeData(0x08); writeData(0x26); writeData(0x2A);
  writeCommand(0xF1); writeData(0x43); writeData(0x70); writeData(0x72); writeData(0x36); writeData(0x37); writeData(0x6F);
  writeCommand(0xF2); writeData(0x45); writeData(0x09); writeData(0x08); writeData(0x08); writeData(0x26); writeData(0x2A);
  writeCommand(0xF3); writeData(0x43); writeData(0x70); writeData(0x72); writeData(0x36); writeData(0x37); writeData(0x6F);
  writeCommand(0xED); writeData(0x1B); writeData(0x0B);
  writeCommand(0xAE); writeData(0x77);
  writeCommand(0xCD); writeData(0x63);
  writeCommand(0x70); writeData(0x07); writeData(0x07); writeData(0x04); writeData(0x0E); writeData(0x0F); writeData(0x09); writeData(0x07); writeData(0x08); writeData(0x03);
  writeCommand(0xE8); writeData(0x34);
  writeCommand(0x62); writeData(0x18); writeData(0x0D); writeData(0x71); writeData(0xED); writeData(0x70); writeData(0x70); writeData(0x18); writeData(0x0F); writeData(0x71); writeData(0xEF); writeData(0x70); writeData(0x70);
  writeCommand(0x63); writeData(0x18); writeData(0x11); writeData(0x71); writeData(0xF1); writeData(0x70); writeData(0x70); writeData(0x18); writeData(0x13); writeData(0x71); writeData(0xF3); writeData(0x70); writeData(0x70);
  writeCommand(0x64); writeData(0x28); writeData(0x29); writeData(0xF1); writeData(0x01); writeData(0xF1); writeData(0x00); writeData(0x07);
  writeCommand(0x66); writeData(0x3C); writeData(0x00); writeData(0xCD); writeData(0x67); writeData(0x45); writeData(0x45); writeData(0x10); writeData(0x00); writeData(0x00); writeData(0x00);
  writeCommand(0x67); writeData(0x00); writeData(0x3C); writeData(0x00); writeData(0x00); writeData(0x00); writeData(0x01); writeData(0x54); writeData(0x10); writeData(0x32); writeData(0x98);
  writeCommand(0x74); writeData(0x10); writeData(0x85); writeData(0x80); writeData(0x00); writeData(0x00); writeData(0x4E); writeData(0x00);
  writeCommand(0x98); writeData(0x3e); writeData(0x07);
  writeCommand(0x35);
  writeCommand(0x21);
  writeCommand(0x11);
  delay(120);
  writeCommand(0x29);
  delay(20);
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
      writeData(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_BGR);
      break;
    case 3: // Inverted landscape
      writeData(TFT_MAD_MV | TFT_MAD_MY | TFT_MAD_BGR);
      break;
  }

  CS_H;
}
