/*
TinyScreen.cpp - Last modified 23 February 2023

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Written by Ben Rose for TinyCircuits.

The latest version of this library can be found at https://tiny-circuits.com/
*/

#include "TinyScreen.h"
#include <avr/pgmspace.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>
#include <Wire.h>

//These delays are used to allow hardware drawing commands on the SSD1331 to execute
#ifndef TS_USE_DELAY
#define TS_USE_DELAY true
#endif

static int xOff, yOff;
static int csPin, dcPin, sclkPin, mosiPin, rstPin;
static int colSetCommand;
static int rowSetCommand;
static int writeRamCommand;
static int cgramOff;
static bool swapBytes;

/*
SPI optimization defines for known architectures
*/

#if defined(ARDUINO_ARCH_AVR)
  #define TS_SPI_SET_DATA_REG(x) SPDR=(x)
#elif defined(ARDUINO_ARCH_SAMD)
  #define TS_SPI_SET_DATA_REG(x) if(_externalIO){SERCOM1->SPI.DATA.bit.DATA=(x);}else{SERCOM4->SPI.DATA.bit.DATA=(x);}
#elif defined(ARDUINO_ARCH_ESP8266)
  #define TS_SPI_SET_DATA_REG(x) SPI1W0 = (x); SPI1CMD |= SPIBUSY
#elif defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)

#include "hardware/dma.h"
#include "rp2040_pio_spi.h"

#include "SSD1357_commands.h"
#include "ST7789_commands.h"
#include "GC9A01_commands.h"

#define TFT_MAD_MY  0x80
#define TFT_MAD_MX  0x40
#define TFT_MAD_MV  0x20
#define TFT_MAD_ML  0x10
#define TFT_MAD_RGB 0x00
#define TFT_MAD_BGR 0x08

#include "SetXY_ST7789.h"
#include "SetXY_SSD1357.h"
#include "SetXY_GC9A01.h"

  #define TS_SPI_SET_DATA_REG(x) write8(x)

#else
  #define TS_SPI_SET_DATA_REG(x) TSSPI->transfer(x)
#endif

#if defined(ARDUINO_ARCH_AVR)
  #define TS_SPI_SEND_WAIT() while(!(SPSR & _BV(SPIF)))
#elif defined(ARDUINO_ARCH_SAMD)
  #define TS_SPI_SEND_WAIT() if(_externalIO){while(SERCOM1->SPI.INTFLAG.bit.DRE == 0);}else{while(SERCOM4->SPI.INTFLAG.bit.DRE == 0);}
#elif defined(ARDUINO_ARCH_ESP8266)
  #define TS_SPI_SEND_WAIT() while(SPI1CMD & SPIBUSY)
#else
  #define TS_SPI_SEND_WAIT() if(0)
#endif

static void (*setWindow)(const int32_t, const int32_t, const int32_t, const int32_t);
static void (*setXFunc)(const int32_t, const int32_t);
static void (*setYFunc)(const int32_t, const int32_t);

/*
TinyScreen uses an I2C GPIO chip to interface with the OLED control lines and buttons
TinyScreen+ has direct IO and uses the Arduino digital IO interface
writeGPIO(address, data);//write to SX1505
startCommand();//write SSD1331 chip select active with data/command signalling a command
startData();//write SSD1331 chip select active with data/command signalling data
endTransfer();//write SSD1331 chip select inactive
getButtons();//read button states, return as four LSBs in a byte- optional button mask
*/

void TinyScreen::writeGPIO(uint8_t regAddr, uint8_t regData) {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)

#elif defined(ARDUINO_ARCH_AVR)
  uint8_t oldTWBR=TWBR;
  TWBR=0;
#endif
  Wire.beginTransmission(GPIO_ADDR+_addr);
  Wire.write(regAddr);
  Wire.write(regData);
  Wire.endTransmission();
#if defined(ARDUINO_ARCH_AVR)
  TWBR=oldTWBR;
#endif
}

void TinyScreen::startCommand(void) {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  tft_pio->fdebug = pull_stall_mask; while (!(tft_pio->fdebug & pull_stall_mask));
  tft_pio->sm[pio_sm].instr = pio_instr_clr_dc;
  sio_hw->gpio_clr = (1ul << csPin);
#else
  if(_externalIO){
    writeGPIO(GPIO_RegData,GPIO_CMD_START);
  }else{
    digitalWrite(TSP_PIN_DC,LOW);
    digitalWrite(TSP_PIN_CS,LOW);
  }
#endif
}

void TinyScreen::startData(void) {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  tft_pio->sm[pio_sm].instr = pio_instr_set_dc;
  sio_hw->gpio_clr = (1ul << csPin);
#else
  if(_externalIO){
    writeGPIO(GPIO_RegData,GPIO_DATA_START);
  }else{
    digitalWrite(TSP_PIN_DC,HIGH);
    digitalWrite(TSP_PIN_CS,LOW);
  }
#endif
}

void TinyScreen::endTransfer(void) {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  tft_pio->fdebug = pull_stall_mask; while (!(tft_pio->fdebug & pull_stall_mask));
  sio_hw->gpio_set = (1ul << csPin);
#else
  if(_externalIO){
    writeGPIO(GPIO_RegData,GPIO_TRANSFER_END);
  } else {
    digitalWrite(TSP_PIN_CS,HIGH);
  }
#endif
}

uint8_t TinyScreen::getButtons(uint8_t buttonMask) {
  uint8_t buttons=0;
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)

#else
  if(_externalIO) {
    Wire.beginTransmission(GPIO_ADDR+_addr);
    Wire.write(GPIO_RegData);
    Wire.endTransmission();
    Wire.requestFrom(GPIO_ADDR+_addr,1);
    buttons=Wire.read();
    //buttons are active low and MSBs, so flip and shift
    buttons=((~buttons)>>4)&0x0F;
  } else {
    if(!digitalRead(TSP_PIN_BT1))buttons|=0x01;
    if(!digitalRead(TSP_PIN_BT2))buttons|=0x02;
    if(!digitalRead(TSP_PIN_BT3))buttons|=0x04;
    if(!digitalRead(TSP_PIN_BT4))buttons|=0x08;
  }
  if(_flipDisplay) {
    uint8_t flipped=0;
    flipped|=((buttons&TSButtonUpperLeft)<<2);
    flipped|=((buttons&TSButtonUpperRight)>>2);
    flipped|=((buttons&TSButtonLowerLeft)<<2);
    flipped|=((buttons&TSButtonLowerRight)>>2);
    buttons=flipped;
  }
#endif // defined
  return buttons&buttonMask;
}

uint8_t TinyScreen::getButtons(void) {
  return getButtons(TSButtonUpperLeft|TSButtonUpperRight|TSButtonLowerLeft|TSButtonLowerRight);
}

/*
SSD1331 Basics
goTo(x,y);//set OLED RAM to pixel address (x,y) with wrap around at x and y max
setX(x start, x end);//set OLED RAM to x start, wrap around at x end
setY(y start, y end);//set OLED RAM to y start, wrap around at y end
*/

void TinyScreen::goTo(uint8_t x, uint8_t y) {
  if(x>xMax||y>yMax)return;
  setX(x,xMax);
  setY(y,yMax);
}

void TinyScreen::setX(uint8_t x, uint8_t end) {
  startCommand();
  if(x>xMax)x=xMax;
  if(end>xMax)end=xMax;
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  setXFunc(x, end);
#else
  TSSPI->transfer(0x15);//set column
  TSSPI->transfer(x);
  TSSPI->transfer(end);
#endif
  endTransfer();
}

void TinyScreen::setY(uint8_t y, uint8_t end) {
  startCommand();
  if(y>yMax)y=yMax;
  if(end>yMax)end=yMax;
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  setYFunc(y, end);
#else
  TSSPI->transfer(0x75);//set row
  TSSPI->transfer(y);
  TSSPI->transfer(end);
#endif
  endTransfer();
}

/*
Hardware accelerated drawing functions:
clearWindow(x start, y start, width, height);//clears specified OLED controller memory
clearScreen();//clears entire screen
drawRect(x stary, y start, width, height, fill, 8bitcolor);//sets specified OLED controller memory to an 8 bit color, fill is a boolean
drawRect(x stary, y start, width, height, fill, 16bitcolor);//sets specified OLED controller memory to an 8 bit color, fill is a boolean
drawRect(x stary, y start, width, height, fill, red, green, blue);//like above, but uses 6 bit color values. Red and blue ignore the LSB.
drawLine(x1, y1, x2, y2, 8bitcolor);//draw a line from (x1,y1) to (x2,y2) with an 8 bit color
drawLine(x1, y1, x2, y2, 16bitcolor);//draw a line from (x1,y1) to (x2,y2) with an 16 bit color
drawLine(x1, y1, x2, y2, red, green, blue);//like above, but uses 6 bit color values. Red and blue ignore the LSB.
*/

void TinyScreen::clearWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  if(x>xMax||y>yMax)return;
  uint8_t x2=x+w-1;
  uint8_t y2=y+h-1;
  if(x2>xMax)x2=xMax;
  if(y2>yMax)y2=yMax;
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  CS_L;
  setWindow(x, y, x+w-1, y+h-1);
  CS_H;
  CS_L;
  (_bitDepth) ? pushBlock16(0x0000, w*h) : pushBlock8(0x0000, w*h);
  CS_H;
#else
  startCommand();
  TSSPI->transfer(0x25);//clear window
  TSSPI->transfer(x);TSSPI->transfer(y);
  TSSPI->transfer(x2);TSSPI->transfer(y2);
  endTransfer();
#endif
#if TS_USE_DELAY
  delayMicroseconds(400);
#endif
}

void TinyScreen::clearScreen() {
  #if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  if(_type == RP2040TVMini) clearWindow(0,0,96,64);
  else if(_type == RP2040TV) clearWindow(0,0,240,135);
  else if(_type == H8Ball) clearWindow(0, 0, 240, 240);
  #else
  clearWindow(0,0,96,64);
  #endif // defined
}

void TinyScreen::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t f, uint8_t r, uint8_t g, uint8_t b) {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  if(_bitDepth) drawRect(x, y, w, h, f, ((b & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (r >> 3));
  else drawRect(x, y, w, h, f, ((r & 0b11000000) >> 6) | ((g & 0b11100000) >> 3) | (b & 0b11100000));
#else
  if(x>xMax||y>yMax)return;
  uint8_t x2=x+w-1;
  uint8_t y2=y+h-1;
  if(x2>xMax)x2=xMax;
  if(y2>yMax)y2=yMax;
  uint8_t fill=0;
  if(f)fill=1;
  startCommand();
  TSSPI->transfer(0x26);//set fill
  TSSPI->transfer(fill);

  TSSPI->transfer(0x22);//draw rectangle
  TSSPI->transfer(x);TSSPI->transfer(y);
  TSSPI->transfer(x2);TSSPI->transfer(y2);
  //outline
  TSSPI->transfer(b);TSSPI->transfer(g);TSSPI->transfer(r);
  //fill
  TSSPI->transfer(b);TSSPI->transfer(g);TSSPI->transfer(r);
  endTransfer();
#endif
#if TS_USE_DELAY
  delayMicroseconds(400);
#endif
}

void TinyScreen::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t f, uint16_t color) {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  if(x>xMax||y>yMax)return;
  uint8_t x2=x+w-1;
  uint8_t y2=y+h-1;
  if(x2>xMax)x2=xMax;
  if(y2>yMax)y2=yMax;
  if(f)
  {
    startCommand();
    setWindow(x, y, x+w-1, y+h-1);
    endTransfer();
    startData();
    (_bitDepth) ? pushBlock16(color, w*h) : pushBlock8(color, w*h);
    endTransfer();
  }
  else
  {
    drawLine(x, y, x+w, y, color);
    drawLine(x, y+h, x+w, y+h, color);
    drawLine(x, y, x, y+h, color);
    drawLine(x+w, y, x+w, y+h, color);
  }
#else
  uint16_t r,g,b;
  if(_bitDepth){
    r=(color)&0x1F;//five bits
    g=(color>>5)&0x3F;//six bits
    b=(color>>11)&0x1F;//five bits
    r=r<<1;//shift to fill six bits
    g=g<<0;//shift to fill six bits
    b=b<<1;//shift to fill six bits
  }else{
    r=(color)&0x03;//two bits
    g=(color>>2)&0x07;//three bits
    b=(color>>5)&0x07;//three bits
    r|=(r<<4)|(r<<2);//copy to fill six bits
    g|=g<<3;//copy to fill six bits
    b|=b<<3;//copy to fill six bits
  }
  drawRect(x,y,w,h,f,r,g,b);
#endif
}

void TinyScreen::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t r, uint8_t g, uint8_t b) {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  if(_bitDepth) drawLine(x0, y0, x1, y1, ((b & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (r >> 3));
  else drawLine(x0, y0, x1, y1, ((r & 0b11000000) >> 6) | ((g & 0b11100000) >> 3) | (b & 0b11100000));
#else
  if(x0>xMax)x0=xMax;
  if(y0>yMax)y0=yMax;
  if(x1>xMax)x1=xMax;
  if(y1>yMax)y1=yMax;
  startCommand();
  TSSPI->transfer(0x21);//draw line
  TSSPI->transfer(x0);TSSPI->transfer(y0);
  TSSPI->transfer(x1);TSSPI->transfer(y1);
  TSSPI->transfer(b);TSSPI->transfer(g);TSSPI->transfer(r);
  endTransfer();
#if TS_USE_DELAY
  delayMicroseconds(100);
#endif
#endif
}

void TinyScreen::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color) {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  if(x0>xMax)x0=xMax;
  if(y0>yMax)y0=yMax;
  if(x1>xMax)x1=xMax;
  if(y1>yMax)y1=yMax;
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx >> 1;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  // Place at correct start pixel
  startCommand();
  if(steep) {
    setX(y0, y0); // Sub-segment is horizontal
    setY(x0, yMax); // Sub-segment is horizontal
  } else {
    setY(y0, y0); // Sub-segment is vertical
    setX(x0, xMax);
  }
  endTransfer();

  // Start by writing colors
  startData();
  int cnt = 0;
  if(_bitDepth) {
    for (; x0 <= x1; x0++) {
      cnt++;
      err -= dy;
      if (err < 0) {
        // Step change, re-align window
        pushBlock16(color, cnt);
        cnt = 0;
        endTransfer();
        y0 += ystep;
        if(steep) {
          setX(y0, y0); // Sub-segment is horizontal
          setY(x0+1, yMax); // Sub-segment is horizontal
        } else {
          setY(y0, y0); // Sub-segment is vertical
          setX(x0+1, xMax);
        }
        err += dx;
        // Start blasting colors again
        startData();
      }
    }
    pushBlock16(color, cnt);
  } else {
    for (; x0 <= x1; x0++) {
      cnt++;
      err -= dy;
      if (err < 0) {
        // Step change, re-align window
        pushBlock8(color, cnt);
        cnt = 0;
        endTransfer();
        y0 += ystep;
        if(steep)
        {
          setX(y0, y0); // Sub-segment is horizontal
          setY(x0+1, yMax); // Sub-segment is horizontal
        }
        else
        {
          setY(y0, y0); // Sub-segment is vertical
          setX(x0+1, xMax);
        }
        err += dx;
        // Start blasting colors again
        startData();
      }
    }
    pushBlock8(color, cnt);
  }
  //write16(color);
  endTransfer();
#else
  uint16_t r,g,b;
  if(_bitDepth){
    r=(color)&0x1F;//five bits
    g=(color>>5)&0x3F;//six bits
    b=(color>>11)&0x1F;//five bits
    r=r<<1;//shift to fill six bits
    g=g<<0;//shift to fill six bits
    b=b<<1;//shift to fill six bits
  }else{
    r=(color)&0x03;//two bits
    g=(color>>2)&0x07;//three bits
    b=(color>>5)&0x07;//three bits
    r|=(r<<4)|(r<<2);//copy to fill six bits
    g|=g<<3;//copy to fill six bits
    b|=b<<3;//copy to fill six bits
  }
  drawLine(x0,y0,x1,y1,r,g,b);
#endif
}

#undef swap

/*
Pixel manipulation
drawPixel(x,y,color);//set pixel (x,y) to specified color. This is slow because we need to send commands setting the x and y, then send the pixel data.
writePixel(color);//write the current pixel to specified color. Less slow than drawPixel, but still has to ready display for pixel data
writeBuffer(buffer,count);//optimized write of a large buffer of 8 bit data. Must be wrapped with startData() and endTransfer(), but there can be any amount of calls to writeBuffer between.
*/

void TinyScreen::drawPixel(uint8_t x, uint8_t y, uint16_t color) {
  if(x>xMax||y>yMax)return;
  goTo(x,y);
  writePixel(color);
}

void TinyScreen::writePixel(uint16_t color) {
  startData();
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  write16(color);
#else
  if(_bitDepth)
    TSSPI->transfer(color>>8);
  TSSPI->transfer(color);
#endif // defined
  endTransfer();
}

void TinyScreen::writeBuffer(uint8_t *buffer,int count) {
  uint8_t temp;
  TS_SPI_SET_DATA_REG(buffer[0]);
  for(int j=1;j<count;j++){
    temp=buffer[j];
    TS_SPI_SEND_WAIT();
    TS_SPI_SET_DATA_REG(temp);
  }
  TS_SPI_SEND_WAIT();
}

/*
TinyScreen commands
setBrightness(brightness);//sets main current level, valid levels are 0-15
on();//turns display on
off();//turns display off, uses less power
setBitDepth(depth);//boolean- 0 is 8 bit, 1 is 16 bit
setFlip(flip);//done in hardware on the SSD1331. boolean- 0 is normal, 1 is upside down
setMirror(mirror);//done in hardware on the SSD1331. boolean- 0 is normal, 1 is mirrored across Y axis
*/

void TinyScreen::setBrightness(uint8_t brightness) {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)

#else
  if(brightness>15)brightness=15;
  startCommand();
  TSSPI->transfer(0x87);//set master current
  TSSPI->transfer(brightness);
  endTransfer();
#endif // defined
}

void TinyScreen::on(void) {
  startCommand();
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  if(_type == RP2040TVMini){
    digitalWrite(9, HIGH);
    delayMicroseconds(10000);
    writeCommand(0xAF); // DISPLAYON
  }else if(_type == RP2040TV || _type == H8Ball){
    digitalWrite(9, LOW);
    writeCommand(0x29);
  }
#else
  if(!_externalIO){
    digitalWrite(TSP_PIN_SHDN,HIGH);
  }
  delayMicroseconds(10000);
  TSSPI->transfer(0xAF); // DISPLAYON
#endif
  endTransfer();
}

void TinyScreen::off(void) {
  startCommand();
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  if(_type == RP2040TVMini){
    writeCommand(0xAE); // DISPLAYOFF
    digitalWrite(9, LOW);
  }else if(_type == RP2040TV){
    writeCommand(0x28);
    digitalWrite(9, HIGH);
  }
#else
  TSSPI->transfer(0xAE); // DISPLAYON
  if(_externalIO){
    writeGPIO(GPIO_RegData,~GPIO_SHDN);//bost converter off
    //any other write will turn the boost converter back on
  }else{
    digitalWrite(TSP_PIN_SHDN,LOW);//SHDN
  }
#endif
  endTransfer();
}

void TinyScreen::setBitDepth(uint8_t b) {
  _bitDepth=b;
  writeRemap();
}

void TinyScreen::setFlip(uint8_t f) {
  _flipDisplay=f;
  writeRemap();
}

void TinyScreen::setMirror(uint8_t m) {
  _mirrorDisplay=m;
  writeRemap();
}

void TinyScreen::setColorMode(uint8_t cm) {
  _colorMode=cm;
  writeRemap();
}

/*
The SSD1331 remap command sets a lot of driver variables, these are kept in memory
and are all written when a change is made.
*/

void TinyScreen::writeRemap(void) {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  uint8_t remap = 0x24 | 0x11;
  if(_type == RP2040TV) remap = 0x60;
#else
  uint8_t remap=(1<<5)|(1<<2);
#endif
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
    if(_flipDisplay)
      if(_type == RP2040TVMini) remap^=((1<<4)|(1<<1));
      else remap ^= 0x80 ^ 0x40;
    if(_mirrorDisplay)
      if(_type == RP2040TVMini) remap^=(1<<1);
      else remap ^= 0x80;
    if(_bitDepth)
      if(_type == RP2040TVMini) remap|=(1<<6);
      else if(_type == RP2040TV) remap ^= 0; // ST7789 uses a custom command for setting bit depth.
    if(_colorMode)
      if(_type == RP2040TVMini) remap^=(1<<2);
      else remap ^= (1 << 3);
#else
  if(_flipDisplay)
    remap|=((1<<4)|(1<<1));
  if(_mirrorDisplay)
    remap^=(1<<1);
  if(_bitDepth)
    remap|=(1<<6);
  if(_colorMode)
    remap^=(1<<2);
#endif
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  if(_type == RP2040TVMini) writeCommand(0xA0);
  else if(_type == RP2040TV) writeCommand(ST7789_MADCTL);
  writeData(remap);
#else
  startCommand();
  TSSPI->transfer(0xA0); // SETREMAP
  TSSPI->transfer(remap);
  endTransfer();
#endif
}

void TinyScreen::begin(void) {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  pinMode(9, OUTPUT);
  if(_type == RP2040TVMini) {
    digitalWrite(9, LOW);
    mosiPin = RPTVM_PIN_MOSI;
    sclkPin = RPTVM_PIN_SCLK;
    rstPin = RPTVM_PIN_RST;
    dcPin = RPTVM_PIN_DC;
    csPin = RPTVM_PIN_CS;
    swapBytes = true;
  } else if(_type == RP2040TV) {
    digitalWrite(9, HIGH);
    mosiPin = RPTV_PIN_MOSI;
    sclkPin = RPTV_PIN_SCLK;
    rstPin = RPTV_PIN_RST;
    dcPin = RPTV_PIN_DC;
    csPin = RPTV_PIN_CS;
    swapBytes = true;
  } else if(_type == H8Ball) {
    digitalWrite(9, HIGH);
    mosiPin = H8B_PIN_MOSI;
    sclkPin = H8B_PIN_SCLK;
    rstPin = H8B_PIN_RST;
    dcPin = H8B_PIN_DC;
    csPin = H8B_PIN_CS;
    swapBytes = false;
  }

  //tft.init(0);
  // Initialization
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH); // Chip select high (inactive)
  pinMode(dcPin, OUTPUT);
  digitalWrite(dcPin, HIGH); // Data/Command high = data mode
  pinMode(rstPin, OUTPUT);
  digitalWrite(rstPin, HIGH); // Set high, do not share pin with another SPI device
  pioinit(80000000);
  CS_H;

  // Initialization
  digitalWrite(rstPin, HIGH);
  delay(5);
  digitalWrite(rstPin, LOW);
  delay(20);
  digitalWrite(rstPin, HIGH);
  delay(150);
  CS_L;
  if(_type == RP2040TVMini) {
    colSetCommand = SSD1357_CASET;
    rowSetCommand = SSD1357_PASET;
    writeRamCommand = SSD1357_RAMWR;
    cgramOff = 32;
    setWindow = setWindowSSD1357;
    setXFunc = setXSSD1357;
    setYFunc = setYSSD1357;
    xOff = 0;
    yOff = 32;
    SSD1357InitSeq();
    CS_L;
    setWindow(0, 0, 63, 63);
    CS_H;
    clearWindow(0,0,64,64);
    digitalWrite(9, HIGH);
  } else if(_type == RP2040TV) {
    colSetCommand = ST7789_CASET;
    rowSetCommand = ST7789_RASET;
    writeRamCommand = ST7789_RAMWR;
    cgramOff = 0;
    setWindow = setWindowST7789;
    setXFunc = setXST7789;
    setYFunc = setYST7789;
    xOff = 40;
    yOff = 53;
    // Regular TinyTV2 init
    ST7789InitSeq();
    // Set window
    CS_L;
    setWindow(0, 0, 239, 134);
    CS_H;
    CS_L;
    pushBlock16(0x0000, 264*135);
    CS_H;
    digitalWrite(9, LOW);
  } else if(_type == H8Ball) {
    // GC9A01 init
    colSetCommand = GC9A01_CASET;
    rowSetCommand = GC9A01_PASET;
    writeRamCommand = GC9A01_RAMWR;
    cgramOff = 0;
    setWindow = setWindowGC9A01;
    setXFunc = setXGC9A01;
    setYFunc = setYGC9A01;
    xOff = 0;
    yOff = 0;
    GC9A01InitSeq();
    // Set window
    CS_L;
    setWindow(0, 0, 239, 239);
    CS_H;
    CS_L;
    pushBlock16(0x0000, 240*240);
    CS_H;
    digitalWrite(9, LOW);
  } else {
    // No valid initialization
  }
#else
  //init SPI
  TSSPI->begin();
  TSSPI->setDataMode(SPI_MODE0);//wrong mode, works because we're only writing. this mode is compatible with SD cards.
#if defined(ARDUINO_ARCH_AVR)
  TSSPI->setClockDivider(SPI_CLOCK_DIV2);
#elif defined(ARDUINO_ARCH_SAMD)
  TSSPI->setClockDivider(4);
#endif

  if(_externalIO){
    //standard TinyScreen- setup GPIO, reset SSD1331
    writeGPIO(GPIO_RegData,~GPIO_RES);//reset low, other pins high
    writeGPIO(GPIO_RegDir,~GPIO_RES);//set reset to output
    delay(5);
    writeGPIO(GPIO_RegDir,~(GPIO_CS|GPIO_DC|GPIO_SHDN));//reset to input, CS/DC/SHDN output
    writeGPIO(GPIO_RegPullUp,GPIO_BTN1|GPIO_BTN2|GPIO_BTN3|GPIO_BTN4);//button pullup enable
  }else{
    //otherwise TinyScreen+, connected directly to IO pins
    pinMode(TSP_PIN_SHDN,OUTPUT);pinMode(TSP_PIN_DC,OUTPUT);pinMode(TSP_PIN_CS,OUTPUT);pinMode(TSP_PIN_RST,OUTPUT);
    digitalWrite(TSP_PIN_SHDN,LOW);digitalWrite(TSP_PIN_DC,HIGH);digitalWrite(TSP_PIN_CS,HIGH);digitalWrite(TSP_PIN_RST,HIGH);
    pinMode(TSP_PIN_BT1,INPUT_PULLUP);pinMode(TSP_PIN_BT2,INPUT_PULLUP);pinMode(TSP_PIN_BT3,INPUT_PULLUP);pinMode(TSP_PIN_BT4,INPUT_PULLUP);
    //reset
    digitalWrite(TSP_PIN_RST,LOW);
    delay(5);
    digitalWrite(TSP_PIN_RST,HIGH);
  }
  delay(10);

  //datasheet SSD1331 init sequence
  constexpr uint8_t init[32]={0xAE, 0xA1, 0x00, 0xA2, 0x00, 0xA4, 0xA8, 0x3F,
  0xAD, 0x8E, 0xB0, 0x0B, 0xB1, 0x31, 0xB3, 0xF0, 0x8A, 0x64, 0x8B,
  0x78, 0x8C, 0x64, 0xBB, 0x3A, 0xBE, 0x3E, 0x81, 0x91, 0x82, 0x50, 0x83, 0x7D};
  off();
  startCommand();
  for(uint8_t i=0;i<32;i++)
    TSSPI->transfer(init[i]);
  endTransfer();
  //use libarary functions for remaining init
  setBrightness(5);
  writeRemap();
  clearWindow(0,0,96,64);
  on();
#endif // defined
}

/*
TinyScreen constructor
type tells us if we're using a regular TinyScreen, alternate addresss TinyScreen, or a TinyScreen+
address sets I2C address of SX1505 to 0x20 or 0x21, which is set by the position of a resistor near SX1505 (see schematic and board design)
*/

TinyScreen::TinyScreen(uint8_t type) {
  _externalIO=0;
  _cursorX=0;
  _cursorY=0;
  _fontHeight=0;
  _fontFirstCh=0;
  _fontLastCh=0;
  _fontDescriptor=0;
  _fontBitmap=0;
  _fontColor=0xFFFF;
  _fontBGcolor=0x0000;
  _bitDepth=0;
  _flipDisplay=0;
  _mirrorDisplay=0;
  _colorMode=0;
  _type=type;

  //type determines the SPI interface IO configuration
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  if(_type==RP2040TV){
    xMax = 239;
    yMax = 134;
  } else if(_type==RP2040TVMini) {
    xMax = 63;
    yMax = 63;
  }
#else
  if(_type==TinyScreenDefault) {
    TSSPI=&SPI;
    _externalIO=1;
    _addr=0;
    xMax = 95;
    yMax = 63;
  } else if(_type==TinyScreenAlternate) {
    TSSPI=&SPI;
    _externalIO=1;
    _addr=1;
    xMax = 95;
    yMax = 63;
  } else if(_type==TinyScreenPlus) {
#if defined(ARDUINO_ARCH_SAMD)
    TSSPI=&SPI1;
#endif
    _externalIO=0;
    xMax = 95;
    yMax = 63;
  } else {
    TSSPI=&SPI;
    _externalIO=1;
    _addr=0;
    xMax = -1;
    yMax = -1;
  }
#endif
}

/*
TinyScreen Text Display
setCursor(x,y);//set text cursor position to (x,y)
setFont(descriptor);//set font data to use
fontColor(text color, background color);//sets text and background color
getFontHeight();//returns height of font

getStringWidth
*/

void TinyScreen::setCursor(uint8_t x, uint8_t y) {
  _cursorX=x;
  _cursorY=y;
}

void TinyScreen::setFont(const FONT_INFO& fontInfo) {
  _fontHeight=fontInfo.height;
  _fontFirstCh=fontInfo.startCh;
  _fontLastCh=fontInfo.endCh;
  _fontDescriptor=fontInfo.charDesc;
  _fontBitmap=fontInfo.bitmap;
}

void TinyScreen::fontColor(uint16_t f, uint16_t g) {
  _fontColor=f;
  _fontBGcolor=g;
}

uint8_t TinyScreen::getFontHeight(const FONT_INFO& fontInfo) {
  return fontInfo.height;
}

uint8_t TinyScreen::getFontHeight(){
  return _fontHeight;
}

uint8_t TinyScreen::getPrintWidth(char * st) {
  if(!_fontFirstCh)return 0;
  uint8_t i,amtCh,totalWidth;
  totalWidth=0;
  amtCh=strlen(st);
  for(i=0;i<amtCh;i++) {
    totalWidth+=pgm_read_byte(&_fontDescriptor[st[i]-_fontFirstCh].width)+1;
  }
  return totalWidth;
}

size_t TinyScreen::write(uint8_t ch) {
  if(!_fontFirstCh)return 1;
  if(ch<_fontFirstCh || ch>_fontLastCh)return 1;
  if(_cursorX>xMax || _cursorY>yMax)return 1;
  uint8_t chWidth=pgm_read_byte(&_fontDescriptor[ch-_fontFirstCh].width);
  uint8_t bytesPerRow=chWidth/8;
  if(chWidth>bytesPerRow*8)
    bytesPerRow++;
  uint16_t offset=pgm_read_word(&_fontDescriptor[ch-_fontFirstCh].offset)+(bytesPerRow*_fontHeight)-1;

  setX(_cursorX,_cursorX+chWidth+1);
  setY(_cursorY,_cursorY+_fontHeight);
  startData();
  for(uint8_t y=0; y<_fontHeight && y+_cursorY<yMax+1; y++){
    if(_bitDepth){

      TS_SPI_SET_DATA_REG(_fontBGcolor>>8);
      TS_SPI_SEND_WAIT();
    }
    TS_SPI_SET_DATA_REG(_fontBGcolor);
    for(uint8_t byte=0; byte<bytesPerRow; byte++) {
      uint8_t data=pgm_read_byte(_fontBitmap+offset-y-((bytesPerRow-byte-1)*_fontHeight));
      uint8_t bits=byte*8;
        for(uint8_t i=0; i<8 && (bits+i)<chWidth && (bits+i+_cursorX)<xMax; i++){
          TS_SPI_SEND_WAIT();
          if(data&(0x80>>i)){
            if(_bitDepth){
              TS_SPI_SET_DATA_REG(_fontColor>>8);
              TS_SPI_SEND_WAIT();
            }
            TS_SPI_SET_DATA_REG(_fontColor);
           }else{
            if(_bitDepth){
              TS_SPI_SET_DATA_REG(_fontBGcolor>>8);
              TS_SPI_SEND_WAIT();
            }
            TS_SPI_SET_DATA_REG(_fontBGcolor);
          }
      }
    }
    TS_SPI_SEND_WAIT();
    if((_cursorX+chWidth)<xMax) {
      if(_bitDepth){
        TS_SPI_SET_DATA_REG(_fontBGcolor>>8);
        TS_SPI_SEND_WAIT();
      }
      TS_SPI_SET_DATA_REG(_fontBGcolor);
      TS_SPI_SEND_WAIT();
    }
  }
  endTransfer();
  _cursorX+=(chWidth+1);
  return 1;
}

/*
TinyScreen+ SAMD21 DMA write
Example code taken from https://github.com/manitou48/ZERO/blob/master/SPIdma.ino
Thanks manitou!
This code is experimental
*/


#if defined(ARDUINO_ARCH_SAMD)

typedef struct {
  uint16_t btctrl;
  uint16_t btcnt;
  uint32_t srcaddr;
  uint32_t dstaddr;
  uint32_t descaddr;
} dmacdescriptor ;
volatile dmacdescriptor wrb[12] __attribute__ ((aligned (16)));
dmacdescriptor descriptor_section[12] __attribute__ ((aligned (16)));
dmacdescriptor descriptor __attribute__ ((aligned (16)));

const uint32_t DMAchannel = 0;
volatile uint32_t dmaReady=true;


void DMAC_Handler() {
  // interrupts DMAC_CHINTENCLR_TERR DMAC_CHINTENCLR_TCMPL DMAC_CHINTENCLR_SUSP
  uint8_t active_channel;

  // disable irqs ?
  __disable_irq();
  active_channel =  DMAC->INTPEND.reg & DMAC_INTPEND_ID_Msk; // get channel number
  DMAC->CHID.reg = DMAC_CHID_ID(active_channel);
  if(DMAC->CHINTFLAG.reg) dmaReady=true;
  DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_TCMPL; // clear
  DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_TERR;
  DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_SUSP;
  __enable_irq();
}

#endif

uint8_t TinyScreen::getReadyStatusDMA() {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  return !(dma_channel_is_busy(dma_tx_channel));
#elif defined(ARDUINO_ARCH_SAMD)
  return dmaReady;
#else
  //it's tough to raise an error about not having DMA in the IDE- try to fall back to regular software transfer
  return 1;//always return ready
#endif
}

void TinyScreen::writeBufferDMA(uint8_t *txdata,int n) {
  if (n == 0) return;
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  //write16(0xFFFF);
  while (dma_channel_is_busy(dma_tx_channel));

  channel_config_set_bswap(&dma_tx_config, swapBytes);
  channel_config_set_transfer_data_size(&dma_tx_config, DMA_SIZE_16);
  //fifoWait(4);
  write8(*txdata);
  txdata++;
  tft_pio->sm[pio_sm].instr = pio_instr_jmp8;
  //TX_FIFO = 0;
  dma_channel_configure(dma_tx_channel, &dma_tx_config, &tft_pio->txf[pio_sm], (uint16_t*)txdata, (n>>1), true);
  stallWait();
#elif defined(ARDUINO_ARCH_SAMD)
  while(!dmaReady);
  uint32_t temp_CHCTRLB_reg;
  // set up transmit channel
  DMAC->CHID.reg = DMAC_CHID_ID(DMAchannel);
  DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
  DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
  DMAC->SWTRIGCTRL.reg &= (uint32_t)(~(1 << DMAchannel));
  if(_externalIO) {
    temp_CHCTRLB_reg = DMAC_CHCTRLB_LVL(0) | DMAC_CHCTRLB_TRIGSRC(SERCOM1_DMAC_ID_TX) | DMAC_CHCTRLB_TRIGACT_BEAT;
  } else {
    temp_CHCTRLB_reg = DMAC_CHCTRLB_LVL(0) | DMAC_CHCTRLB_TRIGSRC(SERCOM4_DMAC_ID_TX) | DMAC_CHCTRLB_TRIGACT_BEAT;
  }
  DMAC->CHCTRLB.reg = temp_CHCTRLB_reg;
  DMAC->CHINTENSET.reg = DMAC_CHINTENSET_MASK ; // enable interrupts
  descriptor.descaddr = 0;
  if(_externalIO) {
    descriptor.dstaddr = (uint32_t) &SERCOM1->SPI.DATA.reg;
  } else {
    descriptor.dstaddr = (uint32_t) &SERCOM4->SPI.DATA.reg;
  }
  descriptor.btcnt =  n;
  descriptor.srcaddr = (uint32_t)txdata;
  descriptor.btctrl =  DMAC_BTCTRL_VALID;
  descriptor.srcaddr += n;
  descriptor.btctrl |= DMAC_BTCTRL_SRCINC;
  memcpy(&descriptor_section[DMAchannel],&descriptor, sizeof(dmacdescriptor));

  dmaReady = false;

  // start channel
  DMAC->CHID.reg = DMAC_CHID_ID(DMAchannel);
  DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;

  //DMAC->CHID.reg = DMAC_CHID_ID(chnltx);   //disable DMA to allow lib SPI- necessary? needs to be done after completion
  //DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
#else
  //it's tough to raise an error about not having DMA in the IDE- try to fall back to regular software transfer
  writeBuffer(txdata,n);//just write the data without DMA
#endif
}

void TinyScreen::initDMA(void) {
#if defined(ARDUINO_ARCH_MBED_RP2040) | defined(ARDUINO_ARCH_RP2040)
  dma_tx_channel = dma_claim_unused_channel(false);

  if (dma_tx_channel < 0) return;

  dma_tx_config = dma_channel_get_default_config(dma_tx_channel);

  //channel_config_set_transfer_data_size(&dma_tx_config, DMA_SIZE_16);
  channel_config_set_dreq(&dma_tx_config, pio_get_dreq(tft_pio, pio_sm, true));
#elif defined(ARDUINO_ARCH_SAMD)
  //probably on by default
  PM->AHBMASK.reg |= PM_AHBMASK_DMAC ;
  PM->APBBMASK.reg |= PM_APBBMASK_DMAC ;
  NVIC_EnableIRQ( DMAC_IRQn ) ;

  DMAC->BASEADDR.reg = (uint32_t)descriptor_section;
  DMAC->WRBADDR.reg = (uint32_t)wrb;
  DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xf);
#else
  //it's tough to raise an error about not having DMA in the IDE- try to fall back to regular software transfer
  //ignore init
#endif
}
