/**
 Send Morse code to GPIO pin.

 Send text to slave via optional nRF24L01 radio.

 @author topquark22
*/

#include "morse.h"

bool transmitMode;

bool testMode;

bool easterMode;

bool radioEnabled;

void setup() {

  Serial.begin(BAUD_RATE);

  initNewArduino();

  // pushbutton
  pinMode(PIN_BUTTON_, INPUT_PULLUP);

  // wire to GND to disable radio
  pinMode(PIN_DISABLE_, INPUT_PULLUP);
  radioEnabled = digitalRead(PIN_DISABLE_);
  
  // Indicator LEDs
  pinMode(PIN_RED, OUTPUT); // Red (error condition)
  pinMode(PIN_OUT, OUTPUT); // Green (transmitted/received data)
  pinMode(PIN_OUT_, OUTPUT); // inverted Green (transmitted/received data)

  digitalWrite(PIN_OUT, LOW);
  digitalWrite(PIN_OUT_, HIGH);

  // device ID jumpers
  pinMode(PIN_ID1, INPUT_PULLUP);

  // device ID jumpers
  pinMode(PIN_ID1, INPUT_PULLUP);

  pinMode(PIN_XRMODE, INPUT_PULLUP);
  transmitMode = digitalRead(PIN_XRMODE);

  pinMode(PIN_PWM, INPUT);

  int pwm = getPWM();
  if (pwm < 255) {
    Serial.print(F("Warning: PWM = ")); Serial.println(pwm);
  }

  // PULLUP makes no difference on pins 6 and 7.
  // So there is a small random chance of starting in easter egg mode
  pinMode(PIN_EASTER_, INPUT);
  easterMode = (0 == analogRead(PIN_EASTER_));
  if (easterMode) {
      Serial.println(F("Easter egg mode"));
  }
  
  readSpeedFromEEPROM();
  readPauseFromEEPROM();
  readMessageFromEEPROM();
  
  // device ID jumpers
  pinMode(PIN_ID1, INPUT_PULLUP);
  pinMode(PIN_ID2, INPUT_PULLUP);

// power level jumpers
  pinMode(PIN_PWR2, INPUT_PULLUP);
  pinMode(PIN_PWR1, INPUT_PULLUP);

  // channel jumpers
  pinMode(PIN_CH10, INPUT_PULLUP);
  pinMode(PIN_CH20, INPUT_PULLUP);

  if (radioEnabled) {
    setupRadio();
  } else {
    Serial.println(F("Radio disabled"));
  }

  pinMode(PIN_TEST_, INPUT_PULLUP);
  testMode = !digitalRead(PIN_TEST_);
  if (testMode) {
    testRoutine();
  }

  if (transmitMode) {
    Serial.println(F("Configured as master\n"));
    showInstructions();
  } else {
    Serial.println(F("Configured as slave\n"));
    if (!radioEnabled) {
      Serial.println(F("Unsupported configuration"));
      errExit();
    }
  }
}

void loop() {
  if (transmitMode) {
    loop_XMIT();
  } else {
    loop_RECV();
  }
}
