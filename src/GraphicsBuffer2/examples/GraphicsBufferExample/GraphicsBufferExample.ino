#include <Wire.h>
#include <SPI.h>
#include <Wireling.h>
#include <TinierScreen.h>
#include <GraphicsBuffer.h>

//TinierScreen display = TinierScreen(TinierScreen042);
//TinierScreen display = TinierScreen(TinierScreen069);
TinierScreen display = TinierScreen(TinierScreen096);

//GraphicsBuffer screenBuffer = GraphicsBuffer(72, 40, colorDepth1BPP);
//GraphicsBuffer screenBuffer = GraphicsBuffer(96, 16, colorDepth1BPP);
GraphicsBuffer screenBuffer = GraphicsBuffer(128, 64, colorDepth1BPP);

int displayPort = 0;
int resetPin = A0+displayPort;

void setup() {
  Wire.begin();
  Wireling.begin();

  Wireling.selectPort(displayPort);
  display.begin(resetPin);


  if (screenBuffer.begin()) {
    //memory allocation error- buffer too big!
  }
  
  screenBuffer.setFont(thinPixel7_10ptFontInfo);
}


int increment = 0;
int xMax, yMax, x, y;
void loop() {
  xMax = screenBuffer.width + 20 - screenBuffer.getPrintWidth("Text Test!");
  yMax = screenBuffer.height + 8 - screenBuffer.getFontHeight();
  x = increment % xMax; if ((increment / xMax) & 1) x = xMax - x;
  y = increment % yMax; if ((increment / yMax) & 1) y = yMax - y;
  x -= 10;
  y -= 4;

  Wireling.selectPort(displayPort);
  screenBuffer.clear();
  screenBuffer.setCursor(x, y);
  screenBuffer.print("Text Test!");
  Wire.setClock(1000000);
  display.writeBuffer(screenBuffer.getBuffer(), screenBuffer.getBufferSize());
  Wire.setClock(50000);

  increment++;
  delay(10);
}
