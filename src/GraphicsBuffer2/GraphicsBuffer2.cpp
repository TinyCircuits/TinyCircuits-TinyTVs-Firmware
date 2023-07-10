/*
TinierScreen.cpp - Last modified 6 January 2020

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

The latest version of this library can be found at https://TinyCircuits.com/
*/


#include "GraphicsBuffer2.h"
#include <avr/pgmspace.h>

int GraphicsBuffer2::begin(void) {
  _cursorX = 0;
  _cursorY = 0;
  _cursorXmin = 0;
  _cursorYmin = 0;
  _cursorXmax = 0;
  _cursorYmax = 0;
  _fontHeight = 0;
  //_fontWidth = 0;??????
  _fontFirstCh = 0;
  _fontLastCh = 0;
  _fontDescriptor=0;
  _fontBitmap=0;
  _fontColor=0xFFFF;
  _fontBGcolor=ALPHA_COLOR;
  _colorMode = 0;
  _bufferSize = width*height;
  if(_bitDepth==16)_bufferSize*=2;
  if(_bitDepth==1)_bufferSize/=8;
  bufferData=NULL;
  //bufferData=(uint8_t *)malloc(_bufferSize);
  //if(!bufferData){
  //  return 1;
  //}
  return 0;
}

void GraphicsBuffer2::setWidth(uint16_t w) {
  width = w;
  _xMax = width-1;
}


void GraphicsBuffer2::goTo(uint8_t x, uint8_t y) {
  if(x>_xMax||y>_yMax)return;
  setX(x,_xMax);
  setY(y,_yMax);
}

void GraphicsBuffer2::setX(uint8_t x, uint8_t end) {
  if(x>_xMax)x=_xMax;
  if(end>_xMax)end=_xMax;
  _cursorX = x;
  _pixelXinc = x;
  _cursorXmin = x;
  _cursorXmax = end;
}

void GraphicsBuffer2::setY(uint8_t y, uint8_t end) {
  if(y>_yMax)y=_yMax;
  if(end>_yMax)end=_yMax;
  _cursorY = y;
  _pixelYinc = y;
  _cursorYmin = y;
  _cursorYmax = end;
}


void GraphicsBuffer2::clearWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {  
  if(x>_xMax||y>_yMax)return;
  uint8_t x2=x+w-1;
  uint8_t y2=y+h-1;
  if(x2>_xMax)x2=_xMax;
  if(y2>_yMax)y2=_yMax;
  
  setX(x,x+w);
  setY(y,y+h);
  for(int i=0;i<w*h;i++)
    writePixel(0);
}

void GraphicsBuffer2::clear(){
  memset(bufferData, 0x00, _bufferSize);
}


void GraphicsBuffer2::setBuffer(uint8_t* buf){
  bufferData=buf;
}

uint8_t* GraphicsBuffer2::getBuffer(){
  return bufferData;
}

uint16_t GraphicsBuffer2::getBufferSize(){
  return _bufferSize;
}

void GraphicsBuffer2::drawPixel(uint8_t x, uint8_t y, uint16_t color)
{
  if(x>_xMax||y>_yMax)return;
  goTo(x,y);
  writePixel(color);
}

void GraphicsBuffer2::writePixel(uint16_t color) {
  uint16_t offset = (uint16_t)(_pixelYinc*(_xMax+1))+(uint16_t)_pixelXinc;
  if(_bitDepth==16){
    bufferData[offset*2] =  color >> 8;
    bufferData[offset*2+1] =  color;
  }else if(_bitDepth==8){
    bufferData[offset] =  color;
  }else if(_bitDepth==1){
    //(_xMax+1)???
    uint16_t pos = _pixelXinc;               //holds the given X-coordinate
    if (_pixelYinc > 7) {               // if Y > 7 (the number of indices in a byte)
      pos += (_pixelYinc/8) * (_xMax+1);    // bump down to the next row by increasing X by the screen width by the number of necessary rows
    }
    uint8_t bitNum = (_pixelYinc % 8);              // adjusts Y such that it can be written within the 0-7 bounds of a byte
    //buffer[pos] |= (1 << (py)); // the bits of the byte within the buffer are set accordingly by placing a 1 in the respective bit location of the byte
    if(color){
      bufferData[pos] |=  (1 << (bitNum));
    }else{
      bufferData[pos] &=  ~(1 << (bitNum));
    }
  }
  incrementPixel();

}

void GraphicsBuffer2::incrementPixel(void) {
  _pixelXinc++;
  if(_pixelXinc>_cursorXmax){
    _pixelXinc = _cursorXmin;
    _pixelYinc++;
    if(_pixelYinc>=_cursorYmax){
      _pixelYinc=_cursorYmin;
    }
  }
}

//Appears to be taken from Adafruit's Bresenham algorithm adaption- Thank you!
#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif
void GraphicsBuffer2::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  if(x0>_xMax)x0=_xMax;
  if(y0>_yMax)y0=_yMax;
  if(x1>_xMax)x1=_xMax;
  if(y1>_yMax)y1=_yMax;
  
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1) {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      drawPixel(y0, x0, color);
    } else {
      drawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void GraphicsBuffer2::drawCircle(int16_t x0, int16_t y0, int16_t radius, uint16_t color){
  int x = radius;
  int y = 0;
  int radiusError = 1 - x;

  while (x >= y)
  {
    drawPixel(x + x0, y + y0, color);
    drawPixel(y + x0, x + y0, color);
    drawPixel(-x + x0, y + y0, color);
    drawPixel(-y + x0, x + y0, color);
    drawPixel(-x + x0, -y + y0, color);
    drawPixel(-y + x0, -x + y0, color);
    drawPixel(x + x0, -y + y0, color);
    drawPixel(y + x0, -x + y0, color);
    y++;
    if (radiusError < 0)
    {
      radiusError += 2 * y + 1;
    }
    else
    {
      x--;
      radiusError += 2 * (y - x) + 1;
    }
  }
}

void GraphicsBuffer2::setCursor(int x, int y){
  _cursorX=x;
  _cursorY=y;
}

void GraphicsBuffer2::setFont(const FONT_INFO& fontInfo){
  _fontHeight=fontInfo.height;
  _fontFirstCh=fontInfo.startCh;
  _fontLastCh=fontInfo.endCh;
  _fontDescriptor=fontInfo.charDesc;
  _fontBitmap=fontInfo.bitmap;
}

void GraphicsBuffer2::fontColor(uint16_t f, uint16_t g){
  _fontColor=f;
  _fontBGcolor=g;
}

uint8_t GraphicsBuffer2::getFontHeight(const FONT_INFO& fontInfo){
  return fontInfo.height;
}

uint8_t GraphicsBuffer2::getFontHeight(){
  return _fontHeight;
}

uint8_t GraphicsBuffer2::getPrintWidth(char * st){
  if(!_fontFirstCh)return 0;
  uint8_t i,amtCh,totalWidth;
  totalWidth=0;
  amtCh=strlen(st);
  for(i=0;i<amtCh;i++){
    totalWidth+=pgm_read_byte(&_fontDescriptor[st[i]-_fontFirstCh].width)+1;
  }
  return totalWidth;
}

size_t GraphicsBuffer2::write(uint8_t ch){
  if(!_fontFirstCh)return 1;
  if(ch<_fontFirstCh || ch>_fontLastCh)return 1;
  if(_cursorX>_xMax || _cursorY>_yMax)return 1;
  uint8_t chWidth=pgm_read_byte(&_fontDescriptor[ch-_fontFirstCh].width);
  
  
  uint8_t ySkip=0;
  if(_cursorY<0){
    if(_cursorY+_fontHeight<0){
      return 1;
    }
    ySkip=0-_cursorY;
  }
  
  uint8_t xSkip=0;
  uint8_t byteSkip=0;
  if(_cursorX<0){
    if(_cursorX+chWidth<0){
     _cursorX+=(int)(chWidth+1);
      return 1;
    }
    xSkip=0-_cursorX;
    //todo
    //byteSkip=xSkip/8;
    //if(byteSkip){
    //  xSkip=
    //}
  }
  uint8_t bytesPerRow=chWidth/8;
  if(chWidth>bytesPerRow*8)
    bytesPerRow++;
  uint16_t offset=pgm_read_word(&_fontDescriptor[ch-_fontFirstCh].offset)+(bytesPerRow*_fontHeight)-1;
  
  setX(_cursorX+xSkip,_cursorX+chWidth);//had chWidth+1, but not using preceding fontbg column?
  setY(_cursorY+ySkip,_cursorY+_fontHeight);
  
  //startData();
  for(uint8_t y=ySkip; y<_fontHeight && ((y+_cursorY)<(_yMax)); y++){//////was (_yMax+1)
    //writePixel(_fontBGcolor);//
    for(uint8_t byte=0; byte<bytesPerRow; byte++){
      uint8_t data=pgm_read_byte(_fontBitmap+offset-y-((bytesPerRow-byte-1)*_fontHeight));
      uint8_t bits=byte*8;
        for(uint8_t i=xSkip; i<8 && (bits+i)<chWidth && (bits+i+_cursorX)<(_xMax+1); i++){//was (_xMax)
          if(data&(0x80>>i)){
            writePixel(_fontColor);
           }else{
            if(_fontBGcolor == ALPHA_COLOR){
              incrementPixel();
            }else{
               writePixel(_fontBGcolor);
            }
          }
      }
    }
    if((_cursorX+chWidth)<=_xMax){//did not have =
      if(_fontBGcolor == ALPHA_COLOR){
        incrementPixel();
      }else{
         writePixel(_fontBGcolor);
      }
    }
  }
  //endTransfer();
  /*if(_cursorX<0){
    SerialUSB.print(_cursorX);
    SerialUSB.print(" ");
  _cursorX+=(int)(chWidth+1);
    SerialUSB.println(_cursorX);
  }*/
  _cursorY-=ySkip;//restore _cursorY
  _cursorX+=(int)(chWidth+1-xSkip);//needs -xSkip due to running setX which will 0 _cursorX
  return 1;
}