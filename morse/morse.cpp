/**
 Send Morse code to GPIO pin.

 Send text to slave via optional nRF24L01 radio.

 @author topquark22
*/

#include "morse.h"

int t_dot;
int t_dash;
int t_space;
int t_pause;

extern bool radioEnabled;

extern bool transmitMode;

byte message[MESSAGE_SIZE];
int message_len;

byte commBuffer[COMM_BUFFER_SIZE];

TEXT_INTERPRETATION interpretTextAs;

RF24 radio(PIN_CE, PIN_CSN, SPI_SPEED);

void clearMessage() {
  message[0] = 0;
  message_len = 0;
}
void initNewArduino() {
  if (NOT_SET == readIntFromEEPROM(ADDR_SPEED)) { // new Arduino
    setSpeed(t_DOT);
    setPause(t_PAUSE);
    clearMessage();
    writeMessageToEEPROM();
  }
}

void blinkRedLED(int ms) {
  digitalWrite(PIN_RED, HIGH);
  delay(ms);
  digitalWrite(PIN_RED, LOW);
}

void setupRadio() {

  uint64_t deviceID = DEVICE_ID_BASE
                + 0x1 * !digitalRead(PIN_ID1)
                + 0x2 * !digitalRead(PIN_ID2);
  Serial.print("Radio starting as device ");
  Serial.println((int)deviceID & 0x0f);

  // power 0=MIN, 1=LOW, 2=HIGH, 3=MAX
  uint8_t power = 2 * digitalRead(PIN_PWR2) + digitalRead(PIN_PWR1);
  Serial.print("Power set to "); Serial.println(power);

  int channel = CHANNEL_BASE
                + 10 * digitalRead(PIN_CH10)
                + 20 * digitalRead(PIN_CH20);
  Serial.print("Channel set to "); Serial.println(channel);

  radio.begin();

  if (!radio.isChipConnected()) {
    Serial.println("Radio not connected");
    while (1) {
      digitalWrite(PIN_RED, LOW);
      delay(250);
      digitalWrite(PIN_RED, HIGH);
      delay(250);
    }
  }

  // visual indication that radio has started
  blinkRedLED(100);

  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(power);
  radio.setChannel(channel);

  if (transmitMode) {
    Serial.println("Configured as master\n");
    radio.openWritingPipe(deviceID); // Get NRF24L01 ready to transmit
    radio.stopListening();

  } else { // recv mode
    Serial.println("Configured as slave\n");
    if (!radioEnabled) {
      Serial.println("Unsupported configuration");
      errExit();
    }
    radio.openReadingPipe(1, deviceID); // Get NRF24L01 ready to receive
    radio.startListening(); // Listen to see if information received
  }
}

void writeMessageToEEPROM() {
  int i;
  for (i = 0; i < message_len && i < EEPROM_LEN - 1; i++) {
    EEPROM.update(i, message[i]);
  }
  EEPROM.update(i, 0);
}

void printMessage() {
  for (int i = 0; i < message_len; i++) {
    Serial.print((char)message[i]);
    if ((i + 1) % (CONSOLE_WIDTH - 1) == 0) {
      Serial.println();
    }
  }
}

void readMessageFromEEPROM() {
  message_len = 0;
  byte b = EEPROM.read(0);
  while (b != 0 && message_len < MESSAGE_SIZE - 1) {
    message[message_len] = b;
    b = EEPROM.read(++message_len);
  }
  message[message_len] = 0;
}

void writeIntToEEPROM(int addr, int value) {
  EEPROM.update(addr, value & 0xFF);
  value >>= 8;
  EEPROM.update(addr + 1, value & 0xFF);
  value >>= 8;
  EEPROM.update(addr + 2, value & 0xFF);
  value >>= 8;
  EEPROM.update(addr + 3, value & 0xFF);
}

int readIntFromEEPROM(int addr) {
  int value = EEPROM.read(addr + 3);
  value = (value << 8) | EEPROM.read(addr + 2);
  value = (value << 8) | EEPROM.read(addr + 1);
  value = (value << 8) | EEPROM.read(addr);
  return value;
}
void writeSpeedToEEPROM() {
  writeIntToEEPROM(ADDR_SPEED, t_dot);
}

void readSpeedFromEEPROM() {
  t_dot = readIntFromEEPROM(ADDR_SPEED);
  t_dash = 3 * t_dot;
  t_space = 6 * t_dot;
}

void writePauseToEEPROM() {
  writeIntToEEPROM(ADDR_PAUSE, t_pause);
}

void readPauseFromEEPROM() {
  t_pause = readIntFromEEPROM(ADDR_PAUSE);
}

void errExit() {
  digitalWrite(PIN_RED, HIGH);
  delay(100); // allow time to flush serial buffer
  exit(1);
}

int getPWM() {
  double pwmRaw = analogRead(PIN_PWM); // 0 to 2^10 - 1
  return pwmRaw / 4;
}

void setOutput(bool value) {
  int pwmWidth = getPWM();
  analogWrite(PIN_OUT, value * pwmWidth);
  analogWrite(PIN_OUT_, 255 - (value * pwmWidth));
}

bool buttonPressed() {
  return !digitalRead(PIN_BUTTON_);
}

void beep(int beep_ms) {
  setOutput(1);
  delay(beep_ms);
  setOutput(0);
  delay(t_dot);
}

void dot() {
  beep(t_dot);
}

void dash() {
  beep(t_dash);
}

static bool isValidUnary(char c) {
  c = toUpperCase(c);
  return ' ' == c || ('0' <= c && c <= '9');
}

static bool isValidHex(char c) {
  c = toUpperCase(c);
  return isValidUnary(c) || ('A' <= c && c <= 'F');
}

static bool isValidMorse(char c) {
  c = toUpperCase(c);
  return isValidUnary(c) || ('A' <= c && c <= 'Z') || '.' == c || ',' == c || '?' == c || '/' == c || '@' == c;
}

static bool isValidChess(char c) {
  c = toLowerCase(c);
  return ' ' == c || ('a' <= c && c <= 'h') || ('1' <= c && c <= '8');
}

void indicateInvalidChar() {
  Serial.print("*");
  blinkRedLED(t_dot);
}

void displayMorse(char c) {
  if (!isValidMorse(c)) {
    indicateInvalidChar();
  } else {
    c = toUpperCase(c);
    switch (c) {
      case ' ' :
        delay(t_space);
        break;
      case 'A' :
        dot(); dash();
        break;
      case 'B' :
        dash(); dot(); dot(); dot();
        break;
      case 'C' :
        dash(); dot(); dash(); dot();
        break;
      case 'D' :
        dash(); dot(); dot();
        break;
      case 'E' :
        dot();
        break;
      case 'F' :
        dot(); dot(); dash(); dot();
        break;
      case 'G' :
        dash(); dash(); dot();
        break;
      case 'H' :
        dot(); dot(); dot(); dot();
        break;
      case 'I' :
        dot(); dot();
        break;
      case 'J' :
        dot(); dash(); dash(); dash();
        break;
      case 'K' :
        dash(); dot(); dash();
        break;
      case 'L' :
        dot(); dash(); dot(); dot();
        break;
      case 'M' :
        dash(); dash();
        break;
      case 'N' :
        dash(); dot();
        break;
      case 'O' :
        dash(); dash(); dash();
        break;
      case 'P' :
        dot(); dash(); dash(); dot();
        break;
      case 'Q' :
        dash(); dash(); dot(); dash();
        break;
      case 'R' :
        dot(); dash(); dot();
        break;
      case 'S' :
        dot(); dot(); dot();
        break;
      case 'T' :
        dash();
        break;
      case 'U' :
        dot(); dot(); dash();
        break;
      case 'V' :
        dot(); dot(); dot(); dash();
        break;
      case 'W' :
        dot(); dash(); dash();
        break;
      case 'X' :
        dash(); dot(); dot(); dash();
        break;
      case 'Y' :
        dash(); dot(); dash(); dash();
        break;
      case 'Z' :
        dash(); dash(); dot(); dot();
        break;
      case '0' :
        dash(); dash(); dash(); dash(); dash();
        break;
      case '1' :
        dot(); dash(); dash(); dash(); dash();
        break;
      case '2' :
        dot(); dot(); dash(); dash(); dash();
        break;
      case '3' :
        dot(); dot(); dot(); dash(); dash();
        break;
      case '4' :
        dot(); dot(); dot(); dot(); dash();
        break;
      case '5' :
        dot(); dot(); dot(); dot(); dot();
        break;
      case '6' :
        dash(); dot(); dot(); dot(); dot();
        break;
      case '7' :
        dash(); dash(); dot(); dot(); dot();
        break;
      case '8' :
        dash(); dash(); dash(); dot(); dot();
        break;
      case '9' :
        dash(); dash(); dash(); dash(); dot();
        break;
      case '.' :
        dot(); dash(); dot(); dash(); dot(); dash();
        break;
      case ',' :
        dash(); dash(); dot(); dot(); dash(); dash();
        break;
      case '?' :
        dot(); dot(); dash(); dash(); dot(); dot();
        break;
      case '/' :
        dash(); dot(); dot(); dash(); dot();
        break;
      case '@' :
        dot(); dash(); dash(); dot(); dash(); dot();
        break;
    }
  }
  delay(2 * t_dot); // inter-character break is 3 including 1 from last dot/dash
}

int ascToHex(char c) {
  char C = toUpperCase(c);
  int n;
  if ('A' <= C && C <= 'F') {
    n = c - 'A' + 0xA;
  } else if ('0' <= c && c <= '9') {
    n = C - '0';
  } else {
    Serial.print("Invalid nybble "); Serial.print(c);
    return -1;
  }
  return n;
}

void displayHex(char c) {
  if (!isValidHex(c)) {
    indicateInvalidChar();
  } else {
    if (' ' == c) {
      delay(t_space);
      return;
    }
    char C = toUpperCase(c);
    int n = ascToHex(c);
    if (n < 0) {
      Serial.print("Invalid nybble "); Serial.print(c);
      return;
    }
    for (int i = 3; i >= 0; i--) {
      n & (1 << i) ? dash() : dot();
    }
  }
  delay(t_space);
}

/**
   Transmit a number in unary
     - Zero represented by a single dash
     - Number n represented by n dots
     - Recommend keeping 0 <= n <= 9
*/
void displayUnary(char c) {
  if (!isValidUnary(c)) {
    indicateInvalidChar();
  } else {
    if (' ' == c) {
      delay(t_space);
      return;
    }
    int n = ascToHex(c);
    if (n < 0) {
      Serial.print("Invalid digit ");
      Serial.print(c);
      return;
    }
    if (n == 0) {
      dash();
    } else {
      for (int i = 0; i < n; i++) {
        dot();
      }
    }
  }
  delay(2 * t_dot);
}

/**
   display chess coordinates 'A'-'H', '1'-'8' as unary 1-8
*/
void displayChess(char c) {
  if (!isValidChess(c)) {
    indicateInvalidChar();
    delay(t_dot);
  } else {
    if (' ' == c) {
      delay(2 * t_space);
      return;
    }
    char C = toUpperCase(c);
    int n;
    int dly;
    if ('A' <= C && C <= 'H') {
      n = C - 'A' + 1;
      dly = 3 * t_dot;
    } else if ('1' <= C && C <= '8') {
      n = C - '0';
      dly = t_space;
    } else if (',' == C) {
      n = 0;
      dly = 3 * t_space;
    } else {
      Serial.print("Invalid digit "); Serial.print(c);
      return;
    }
    for (int i = 0; i < n; i++) {
      dot();
    }
    delay(dly);
  }
}

bool displayEnabled = true;

void enableDisplay(bool enable) {
  displayEnabled = enable;
}

void displayMessage() {
  if (!displayEnabled || 0 == message_len) {
    return;
  }
  int i;
  char c = message[0];
  for (i = 0; c != 0 && i < message_len; c = message[++i]) {

    if (buttonPressed()) {
        Serial.println("-- Entering test mode");
        testRoutine(); // never returns
    }

    if (!transmitMode) {
      // recv mode
      if (radioEnabled && radio.available()) {
        // abort display when new message comes in
        break;
      }
    } else if (Serial.available()) {
      // abort display to allow entry of new message
      break;
    }

    Serial.print(c);
    if (SW_HEXADECIMAL == c) {
      interpretTextAs = HEXADECIMAL;
      continue;
    } else if (SW_UNARY == c) {
      interpretTextAs = UNARY;
      continue;
    } else if (SW_CHESS == c) {
      interpretTextAs = CHESS;
      continue;
    } else if ('_' == c) {
      interpretTextAs = MORSE;
      continue;
    } else if (0 == i) {
      interpretTextAs = MORSE;
    }
 
    switch (interpretTextAs) {
    case MORSE:
      displayMorse(c);
      break;
    case HEXADECIMAL:
      displayHex(c);
      break;
    case UNARY:
      displayUnary(c);
      break;
    case CHESS:
      displayChess(c);
      break;
    }
    if ((i + 1) % (CONSOLE_WIDTH - 1) == 0) {
      Serial.println();
    }
  }
  Serial.println();
  delay(t_pause);
};

/*
   Sets the duration of 1 dot in ms
*/
void setSpeed(int t_dot_ms) {
  if (t_dot_ms <= 0) {
    Serial.println("Invalid speed");
    return;
  }
  t_dot = t_dot_ms;
  t_dash = 3 * t_dot;
  t_space = 6 * t_dot;
  writeSpeedToEEPROM();
  Serial.print("-- speed set to ");
  Serial.println(t_dot);
}

void setPause(int t_pause_ms) {
  if (t_pause_ms <= 0) {
    Serial.println("invalid pause");
    return;
  }
  t_pause = t_pause_ms;
  writePauseToEEPROM();
  Serial.print("-- pause set to ");
  Serial.println(t_pause);
}
