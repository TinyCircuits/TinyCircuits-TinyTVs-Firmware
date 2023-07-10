static SPIClass* tsspi;

void setXSSD1331(const int32_t x0, const int32_t x1)
{
  tsspi->transfer(0x15);//set column
  tsspi->transfer((uint8_t)x0);
  tsspi->transfer((uint8_t)x1);
}

void setYSSD1331(const int32_t y0, const int32_t y1)
{
  tsspi->transfer(0x75);//set row
  tsspi->transfer((uint8_t)y0);
  tsspi->transfer((uint8_t)y1);
}

/* // Not used for TSP
void setWindowSSD1331(const int32_t y0, const int32_t x0, const int32_t y1, const int32_t x1)
{

}
*/
