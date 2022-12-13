// Pin definitions
const uint8_t BTN_1 = 11;
const uint8_t IR_PIN = 1;

// Pin definitions for SD card
const uint8_t SD_CS = 17;
const uint8_t SD_MISO = 16;
const uint8_t SD_MOSI = 19;
const uint8_t SD_SCK = 18;
const SdSpiConfig SD_CONFIG(SD_CS, DEDICATED_SPI, min(F_CPU / 2, 50000000), &SPI);

// Pin definitions for audio
const uint8_t AUDIO_PIN = 22;
const uint8_t SPK_EN = 23;

const int LEFT_BTN_PIN = 25;
const int RIGHT_BTN_PIN = 0;

// Rotary encoder pins
const int ENCODER_PINA = 3;
const int ENCODER_PINB = 2;
const int ENCODER2_PINA = 25;
const int ENCODER2_PINB = 24;

// Ugly state variables for encoders
volatile int encoderCW = 0;
volatile int encoderCCW = 0;
volatile int encoder2CW = 0;
volatile int encoder2CCW = 0;
int newPinA = 1;
int newPinB = 1;
int oldPinA = 1;
int oldPinB = 1;
int newPinA2 = 1;
int newPinB2 = 1;
int oldPinA2 = 1;
int oldPinB2 = 1;

const int CPU_HZ = F_CPU;

volatile struct TinyIRReceiverCallbackDataStruct sCallbackData;


volatile bool powerButtonWasPressed = false;
volatile bool leftButtonWasPressed = false;
volatile bool rightButtonWasPressed = false;
uint8_t btnState;
uint64_t lastIRInput = 0; // Helpful for dealing with button signal bounce on this end


void initalizePins() {
#ifdef TinyTVMini
  // OLED boost supply pin
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  pinMode(LEFT_BTN_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(LEFT_BTN_PIN), leftButtonPressedInt, FALLING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_BTN_PIN), rightButtonPressedInt, FALLING);
#else

  // TFT LED pin
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);

  pinMode(ENCODER_PINA, INPUT_PULLUP);
  pinMode(ENCODER_PINB, INPUT_PULLUP);
  pinMode(ENCODER2_PINA, INPUT_PULLUP);
  pinMode(ENCODER2_PINB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PINA), encoderPinChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PINB), encoderPinChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER2_PINA), encoder2PinChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER2_PINB), encoder2PinChange, CHANGE);
#endif


  // Software shutoff pin
  pinMode(20, OUTPUT);
  digitalWrite(20, LOW);

  // Power button
  pinMode(BTN_1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_1), powerButtonPressedInt, FALLING);

  // Enable the speaker and set the PWM pin up for audio
  initAudioPin(AUDIO_PIN);

  pinMode(SPK_EN, OUTPUT);
  digitalWrite(SPK_EN, HIGH);
}

int initializeSDcard() {
  // Set SPI up and make sure the SD card is working
  SPI.setTX(SD_MOSI); SPI.setRX(SD_MISO);
  SPI.setSCK(SD_SCK);
  SPI.setCS(SD_CS);
  SPI.begin(true);  //check argument?
  return sd.cardBegin(SD_CONFIG);
}

int initializeFS(){
  return sd.volumeBegin();
}


void handleReceivedTinyIRData(uint16_t aAddress, uint8_t aCommand, bool isRepeat)
{
  sCallbackData.Address = aAddress;
  sCallbackData.Command = aCommand;
  sCallbackData.isRepeat = isRepeat;
  sCallbackData.justWritten = true;
}

void IRInput()
{
  if (sCallbackData.justWritten)
  {
    sCallbackData.justWritten = false;
    lastIRInput = millis();
    if (sCallbackData.Command == 0x1)
      channelUpInput = true;
    if (sCallbackData.Command == 0x4)
      channelDownInput = true;
    if (sCallbackData.Command == 0xF)
      muteInput = true;
    if (sCallbackData.Command == 0xD)
      volDownInput = true;
    if (sCallbackData.Command == 0xE)
      volUpInput = true;
    if (sCallbackData.Command == 0x11)
      powerInput = true;
  }
}



inline void updateButtonStates()
{

  if (powerButtonWasPressed) {
    powerButtonWasPressed = false;
    powerInput = true;
  }
  //  if (digitalRead(BTN_1) == LOW && btnState != LOW)
  //  {
  //    powerInput = true;
  //  }

  btnState = digitalRead(BTN_1);

  // Encoder/button input
  if (encoderCW > 0) {
    encoderCW = 0;
    channelUpInput = true;
  }
  if (encoderCCW > 0) {
    encoderCCW = 0;
    channelDownInput = true;
  }
  if (encoder2CW > 0) {
    encoder2CW = 0;
    volUpInput = true;
  }
  if (encoder2CCW > 0) {
    encoder2CCW = 0;
    volDownInput = true;
  }
  if (leftButtonWasPressed) {
    leftButtonWasPressed = false;
    volUpInput = true;
  }
  if (rightButtonWasPressed) {
    rightButtonWasPressed = false;
    channelUpInput = true;
  }
}

void encoderPinChange() {
  newPinA = digitalRead(ENCODER_PINA);
  newPinB = digitalRead(ENCODER_PINB);
  if (newPinA != oldPinA || newPinB != oldPinB) {
    if ( !oldPinB && !newPinB ) {
      if (!oldPinA &&  newPinA)
        encoderCCW++;
      if (oldPinA &&  !newPinA)
        encoderCW++;
    }
  }
  oldPinA = newPinA;
  oldPinB = newPinB;
}

void encoder2PinChange() {
  newPinA2 = digitalRead(ENCODER2_PINA);
  newPinB2 = digitalRead(ENCODER2_PINB);
  if (newPinA2 != oldPinA2 || newPinB2 != oldPinB2) {
    if ( !oldPinB2 && !newPinB2 ) {
      if (!oldPinA2 &&  newPinA2)
        encoder2CCW++;
      if (oldPinA2 &&  !newPinA2)
        encoder2CW++;
    }
  }
  oldPinA2 = newPinA2;
  oldPinB2 = newPinB2;
}


void leftButtonPressedInt() {
  leftButtonWasPressed = true;
}

void rightButtonPressedInt() {
  rightButtonWasPressed = true;
}

void powerButtonPressedInt() {
  powerButtonWasPressed = true;
}

void clearPowerButtonPressInt() {
  powerButtonWasPressed = false;
}
bool powerButtonPressed() {
  return (digitalRead(BTN_1) == LOW);
}
