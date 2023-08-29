/**
 Send Morse code to GPIO pin.

 Send text to slave via optional nRF24L01 radio.

 @author topquark22
*/

#include "morse.h"

bool transmitMode = digitalRead(PIN_XRMODE);

bool testMode = !digitalRead(PIN_TEST_);

bool radioEnabled = digitalRead(PIN_DISABLE_);

void setup() {

  Serial.begin(BAUD_RATE);

  initNewArduino();

  // pushbutton
  pinMode(PIN_BUTTON_, INPUT_PULLUP);

  // wire to GND to disable radio
  pinMode(PIN_DISABLE_, INPUT_PULLUP);

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

  pinMode(PIN_PWM, INPUT);

  int pwm = getPWM();
  if (pwm < 255) {
    Serial.print("Warning: PWM = "); Serial.println(pwm);
  }

  readSpeedFromEEPROM();
  readPauseFromEEPROM();
  readMessageFromEEPROM();

  // device ID jumpers
  pinMode(PIN_ID1, INPUT_PULLUP);

// power level jumpers
  pinMode(PIN_PWR2, INPUT_PULLUP);
  pinMode(PIN_PWR1, INPUT_PULLUP);

  // channel jumpers
  pinMode(PIN_CH10, INPUT_PULLUP);
  pinMode(PIN_CH20, INPUT_PULLUP);
  pinMode(PIN_CH40, INPUT_PULLUP);

  if (radioEnabled) {
    setupRadio();
  }

  pinMode(PIN_TEST_, INPUT_PULLUP);
  testMode = !digitalRead(PIN_TEST_);
  if (testMode) {
    testRoutine();
  }

  if (transmitMode) {
    showInstructions();
  }
}

void loop() {
  // can enter test mode by holding down button switch
  if (buttonPressed()) {
    testRoutine();
  }
  if (transmitMode) {
    loop_XMIT();
  } else {
    loop_RECV();
  }
}
