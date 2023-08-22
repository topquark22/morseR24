/**
 * Send Morse code to GPIO pin.
 * 
 * Send text to slave via optional nRF24L01 radio.
 * 
 * @version 7.0
 * @author topquark22
 */

#include <RF24_config.h>
#include <nRF24L01.h>
#include <RF24.h>
#import <EEPROM.h>

// Note these choices of CE, CSN are mandatory for integrated Nano3/nRF24l01 boards
// and so should always be used, NOT 9 and 10 as you see in most places!!
const int PIN_CE = 10;
const int PIN_CSN = 9;

const int SPI_SPEED = 10000000;

const int PIN_DISABLE_ = 4; // wire to GND if not using radio

const int PIN_XRMODE = 8; // wire to GND for recv (slave) mode; else xmit (master) mode

const int PIN_TEST_ = 7; // jumper to GND for test mode

const int PIN_BUTTON_ = 6; // code key switch for manual input in test mode

const int PIN_OUT = 5;  // output
const int PIN_OUT_ = 3; // inverted output
const int PIN_RED = 2;  // LED, hemorrhoid condition

// radio output power = 2*A0 + A1
const int PIN_PWR2 = A0;
const int PIN_PWR1 = A1;

// Which RF channel to communicate on, 0-125. Default 118 = 48 + 10 + 20 + 40
const int CHANNEL_BASE = 48;
const int PIN_CH10 = A2; // if not wired low, add 10 to CHANNEL_BASE
const int PIN_CH20 = A3; // if not wired low, add 20 to CHANNEL_BASE
const int PIN_CH40 = A4; // if not wired low, add 40 t0 CHANNEL_BASE

// Device ID setting. Must match transmitter and receiver
const uint64_t DEVICE_ID_BASE = 0x600DFF2430LL;
const int PIN_ID1 = A5; // if wired low, add 0x1 to ID_BASE

// analog input for PWM
// Usually you would wire this to +5V (it's right next to the +5V pin)
const int PIN_PWM = A7;

// defaults
const int t_DOT = 100;
const int t_PAUSE = 3000;

// Serial transmission rate
const int BAUD_RATE = 9600;

// max serial buffer length
const int BUFLEN = 64;

// EEPROM addresses
const int ADDR_SPEED = 0x3F0;
const int ADDR_PAUSE = 0x3F4;

// packet type tokens (first byte of payload)
const int TOKEN_TEST = 1;
const int TOKEN_SPEED = 2;
const int TOKEN_PAUSE = 3;
const int TOKEN_MESSAGE_COMPLETE = 4;
const int TOKEN_MESSAGE_INCOMPLETE = 5;

int t_dot;
int t_dash;
int t_space;
int t_pause;

bool transmitMode; // else receive mode

bool radioEnabled;

bool testMode;

const int PAYLOAD_LEN = 32;
byte msg[PAYLOAD_LEN]; // Used to store/receive message via radio

RF24 radio(PIN_CE, PIN_CSN, SPI_SPEED);

void writeRadio(String message) {
  Serial.println("Writing to radio, message: " + message);
  int i;
  for (int i = 0; i < message.length(); i++) {
    msg[i] = message[i];
  }
  for (int j = i; j < PAYLOAD_LEN; j++) {
    msg[j] = 0;
  }
  radio.write(msg, PAYLOAD_LEN);
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

int parseInt(String s) {
  int res = 0;
  for (int i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (c >= '0' && c <= '9') {
      res = res * 10 + (c - '0');
    } else {
      return -1;
    }
  }
  return res;
}

/**
 * Text interpretation (can using special character mid-message)
 */
enum TEXT_INTERPRETATION {
  MORSE, // '_' in stream
  HEXADECIMAL,  // '$' in stream
  UNARY, // '#' in stream
  CHESS  // '%' in stream
};
TEXT_INTERPRETATION interpretTextAs;

const char SW_MORSE = '_';
const char SW_HEXADECIMAL  = '$';
const char SW_UNARY = '#';
const char SW_CHESS = '%';



// Width of serial console
const int CONSOLE_WIDTH = 100;

const int EEPROM_LEN = 0x3F0; // leave room for speed, pause

void writeMessageToEEPROM(String message) {
  int i;
  for (i = 0; i < message.length() && i < EEPROM_LEN - 1; i++) {
    EEPROM.update(i, message[i]);
  }
  EEPROM.update(i, 0);
}

String readMessageFromEEPROM() {
  String message = "";
  int i = 0;
  byte b = EEPROM.read(0);
  while (i < EEPROM_LEN && b > 0) {
    message += (char)b;
    b = EEPROM.read(++i);
  }
  return message;
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

void displayMorse(char c) {
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
  delay(2 * t_dot); // inter-character break is 3 including 1 from last dot/dash
}

int ascToNybble(char c) {
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

void displayNybble(char c) {
  if (' ' == c) {
    delay(t_space);
    return;
  }
  char C = toUpperCase(c);
  int n = ascToNybble(c);
  if (n < 0) {
    Serial.print("Invalid nybble "); Serial.print(c);
    return;
  }
  for (int i = 3; i >= 0; i--) {
    n & (1 << i) ? dash() : dot();
  }
  delay(t_dot);
}

/**
 * Transmit a number in unary
 *   - Zero represented by a single dash
 *   - Number n represented by n dots
 *   - Recommend keeping 0 <= n <= 9
 */
void displayUnary(char c) {
  if (' ' == c) {
    delay(t_space);
    return;
  }
  int n = ascToNybble(c);
  if (n < 0) {
    Serial.print("Invalid digit "); Serial.print(c);
    return;
  }
  if (n == 0) {
    dash();
  } else {
    for (int i = 0; i < n; i++) {
      dot();
    }
  }
  delay(2 * t_dot);
}

/**
 * display chess coordinates 'A'-'H', '1'-'8' as unary 1-8
 */
void displayChess(char c) {
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

void transmitMessage(String message) {
  if (!radioEnabled) {
    return;
  }
  Serial.println("-- Transmitting message");
  
  msg[0] = TOKEN_MESSAGE_INCOMPLETE;
  // Break messages into chunks of 31 characters
  // First make life easy by making a zero-terminated byte array
  const int BUF_LEN = message.length() + 1;
  byte messageBytes[BUF_LEN];
  int i;
  for (i = 0; i < message.length(); i++) {
    messageBytes[i] = message[i];
  }
  messageBytes[i] = 0;

  const int BLOCK_SIZE = PAYLOAD_LEN - 2;

  int j = 0;
  while (j < BUF_LEN) {
    msg[j % BLOCK_SIZE + 1] = messageBytes[j];
    
    // zero-pad last block (not strictly necessary)
    if (0 == messageBytes[j]) {
      msg[0] = TOKEN_MESSAGE_COMPLETE;
      for (int k = j % BLOCK_SIZE + 2; k < PAYLOAD_LEN - 1; k++) {
        msg[k] = 0;
      }
    }
    
    byte checksum = 0;
    for (int k = 0; k < PAYLOAD_LEN - 1; k++) {
      checksum ^= msg[k];
    }
    
    if (0 == messageBytes[j] ||  (j + 1) % BLOCK_SIZE == 0) {
      msg[PAYLOAD_LEN - 1] = checksum;
      radio.write(msg, PAYLOAD_LEN);
    }
    j++;
  }
}

void displayMessage(String message) {
  int i;
  char c;
  for (i = 0, c = message.charAt(0); c != 0; c = message.charAt(++i)) {
    if ((transmitMode && Serial.available()) || (!transmitMode && radio.available())) {
      break;
    }
    if (SW_HEXADECIMAL == c) {
      if (interpretTextAs != HEXADECIMAL)
        Serial.print(SW_HEXADECIMAL);
      interpretTextAs = HEXADECIMAL;
      continue;
    } else if (SW_UNARY == c) {
      if (interpretTextAs != UNARY)
        Serial.print(SW_UNARY);
      interpretTextAs = UNARY;
      continue;
    } else if (SW_CHESS == c) {
      if (interpretTextAs != CHESS)
        Serial.print(SW_CHESS);
      interpretTextAs = CHESS;
      continue;
    } else if (0 == i || '_' == c) {
      if (interpretTextAs != MORSE)
        Serial.print(SW_MORSE);
      interpretTextAs = MORSE;
      if ('_' == c)
        continue;
    }
    Serial.print(c);
    switch (interpretTextAs) {
      case MORSE:
        displayMorse(c);
        break;
      case HEXADECIMAL:
        displayNybble(c);
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
  if (i > 0) {
    Serial.println();
  }
  interpretTextAs = MORSE;
}

bool buttonPressed() {
  return !digitalRead(PIN_BUTTON_);
}

void testRoutine() {
  Serial.println("Test mode");
  if (buttonPressed()) {
    int prevValue = -1;
    while (1) {
      int value = buttonPressed();
      if (value != prevValue) {
        prevValue = value;
        setOutput(value);
        transmitInteger(TOKEN_TEST, value);
      }
      delay(10);
    }
  } else { // beep at 1 second intervals
    while (1) {
      setOutput(1);
      transmitInteger(TOKEN_TEST, 1);
      setOutput(1);
      delay(1000);
      setOutput(0);
      transmitInteger(TOKEN_TEST, 0);
      setOutput(0);
      delay(1000);
    }
  }
}
void setup() {

  Serial.begin(BAUD_RATE);
  
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
  uint64_t deviceID = DEVICE_ID_BASE + 0x1 * !digitalRead(PIN_ID1);

  pinMode(PIN_XRMODE, INPUT_PULLUP);
  transmitMode = digitalRead(PIN_XRMODE);
  if (transmitMode) {
    Serial.println("Configured as transmitter");
  } else {
    Serial.println("Configured as receiver");
  }
  
  pinMode(PIN_PWM, INPUT);
  int pwm = getPWM();
  if (pwm < 255) {
    Serial.print("Warning: PWM = "); Serial.println(pwm);
  }

  if (0xFF == EEPROM.read(0)) { // new Arduino
    EEPROM.write(0, 0);
    setSpeed(t_DOT);
    setPause(t_PAUSE);
  }
  readSpeedFromEEPROM();
  readPauseFromEEPROM();
    
  if (radioEnabled) {

    Serial.print("Radio starting as device ");
    Serial.println((int)deviceID & 0x0f);
    
    pinMode(PIN_PWR2, INPUT_PULLUP);
    pinMode(PIN_PWR1, INPUT_PULLUP);
    // power 0=MIN, 1=LOW, 2=HIGH, 3=MAX
    uint8_t power = 2 * digitalRead(PIN_PWR2) + digitalRead(PIN_PWR1);
    Serial.print("Power set to "); Serial.println(power);

    // channel jumpers
    pinMode(PIN_CH10, INPUT_PULLUP);
    pinMode(PIN_CH20, INPUT_PULLUP);
    pinMode(PIN_CH40, INPUT_PULLUP);

    int channel = CHANNEL_BASE
                + 10 * digitalRead(PIN_CH10)
                + 20 * digitalRead(PIN_CH20)
                + 40 * digitalRead(PIN_CH40);
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
    digitalWrite(PIN_RED, HIGH);
    delay(100);
    digitalWrite(PIN_RED, LOW);
      
    radio.setDataRate(RF24_1MBPS);
    radio.setPALevel(power);
    radio.setChannel(channel);

    if (transmitMode) {
      Serial.println("Configured as transmitter");
      radio.openWritingPipe(deviceID); // Get NRF24L01 ready to transmit
      radio.stopListening();

      transmitSpeed();
      transmitPause();

    } else { // recv mode
 
      radio.openReadingPipe(1, deviceID); // Get NRF24L01 ready to receive
      radio.startListening(); // Listen to see if information received
      
    }
  } else {
    Serial.println("radio disabled");
  }

  pinMode(PIN_TEST_, INPUT_PULLUP);
  testMode = !digitalRead(PIN_TEST_);
  if (testMode) {
    testRoutine();
  }

  if (transmitMode) {
    
    Serial.print("Dot duration: ");
    Serial.print(t_dot);
    Serial.println(" ms");
    Serial.print("Pause duration: ");
    Serial.print(t_pause);
    Serial.println(" ms");
    Serial.println("Accepting input from serial console");
    Serial.println("Commands:");
    Serial.println("*speed <dot ms>");
    Serial.println("*pause <pause ms>");
    Serial.println();
    Serial.println("In-stream modifiers:");
    Serial.println("_: Morse mode (default)");
    Serial.println("$: Hex mode");
    Serial.println("#: Unary mode");
    Serial.println("%: Chess mode");
    Serial.println();

  } else { // recv mode

    if (testMode) {
      Serial.println("Test mode");
      while(1) {
        bool value = buttonPressed();
        setOutput(value);
        delay(10);
      }
    }

    if (!radioEnabled) {
      Serial.println("Unsupported configuration");
      digitalWrite(PIN_RED, HIGH);
    }
  }
}

/*
 * Sets the duration of 1 dot in ms
 */
void setSpeed(int t_dot_ms) {
  if (t_dot_ms < 0) {
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
  if (t_pause_ms < 0) {
    Serial.println("invalid pause");
    return;
  }
  t_pause = t_pause_ms;
  writePauseToEEPROM();
  Serial.print("-- pause set to ");
  Serial.println(t_pause);
}

void transmitInteger(int tokenType, int value) {
  msg[0] = tokenType;
  msg[1] = (value >> 24) & 0xFF;
  msg[2] = (value >> 16) & 0xFF;
  msg[3] = (value >> 8) & 0xFF;
  msg[4] = value & 0xFF;
  // zero fill rest of packet (not strictly necessary)
  for (int i = 5; i < PAYLOAD_LEN; i++) {
    msg[i] = 0;
  }
  radio.write(msg, PAYLOAD_LEN);
}

void transmitSpeed() {
  transmitInteger(TOKEN_SPEED, t_dot);
}

void transmitPause() {
  transmitInteger(TOKEN_PAUSE, t_pause);
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

String readLine() {

  String line = "";
  while (!Serial.available()) {
    delay(10);
  }
  while (Serial.available()) {
    char c = Serial.read();
    if ('\n' != c && '\r' != c) {
      line = line + c;
    }
  }
  return line;
}

const int INPUT_LEN = 64;

bool messageChanged = true;

void loop_XMIT() {

  if (!Serial.available()) {

    if (messageChanged) {
      String message = readMessageFromEEPROM();
      transmitMessage(message);
      messageChanged = false;
    }
    delay(t_pause);

  } else { // Serial.available()

    String line = readLine();
    if (line.substring(0,1).equals("*")) {
      if (line.substring(0, 7).equals("*speed ")) {
        String speedStr = line.substring(7, line.length());
        int speed = parseInt(speedStr);
        setSpeed(speed);
        transmitSpeed();
        return;
      }
    
      if (line.substring(0, 7).equals("*pause ")) {
        String pauseStr = line.substring(7, line.length());
        int pause = parseInt(pauseStr);
        setPause(pause);
        transmitPause();
        return;
      }
      else {
        Serial.println("-- Invalid * command");
        return;
      }
    }
  
    Serial.println("-- Append to message (max 64 chars per line. Blank line commits message)");
  
    String message = line;
    Serial.println();
    Serial.print(message);
    Serial.println("<");
      
    line = readLine();
    while (line.length() > 0) {
      message = message + line;
      Serial.print(message);
      Serial.println("<");
      line = readLine();
    }

    if (message.length() == 0) {
      Serial.println("-- Message cleared");
    }
    transmitMessage(message);
    writeMessageToEEPROM(message);
    messageChanged = true;
  }
  String message = readMessageFromEEPROM();
  displayMessage(message);
}

String decodeMsg() {
  String message = "";
  for (int i = 1; i < PAYLOAD_LEN - 1 && msg[i] > 0; i++) {
    message = message + (char)msg[i];
  }
  byte checksum = 0;
  for (int i = 0; i < PAYLOAD_LEN - 1; i++) {
    checksum ^= msg[i];
  }
  if (msg[PAYLOAD_LEN - 1] != checksum) {
    digitalWrite(PIN_RED, HIGH);
  }
  return message;
}

String decodeAllPackets() {
  String message = decodeMsg(); // first packet already in buffer
  while (TOKEN_MESSAGE_INCOMPLETE == msg[0]) {
    // expecting another packet; might need to wait for it to arrive
    while (!radio.available()) {
      delay(1);
    }
    radio.read(msg, PAYLOAD_LEN);
    String next = decodeMsg();
    message = message + next;
  }
  return message;
}

bool displayDisabled = false;

void loop_RECV() {
  
  if (radioEnabled) {
    if (radio.available()) {
      digitalWrite(PIN_RED, LOW);
      radio.read(msg, PAYLOAD_LEN); // Read data from the nRF24L01
      if (TOKEN_MESSAGE_COMPLETE == msg[0]) {
        String message = decodeMsg();
        writeMessageToEEPROM(message);
        displayDisabled = false;
      } else if (TOKEN_MESSAGE_INCOMPLETE == msg[0]) {
        String message = decodeAllPackets();
        writeMessageToEEPROM(message);
        displayDisabled = false;
      } else if (TOKEN_TEST == msg[0]) { // special case manual transmission
        setOutput(msg[4]);
        displayDisabled = true;
      } else if (TOKEN_SPEED == msg[0]) { // speed was transmitted
        setSpeed((msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4]);
      } else if (TOKEN_PAUSE == msg[0]) { // pause was transmitted
        setPause((msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4]);
      } else { // invalid token in msg[0]
        Serial.println("Invalid packet received");
        digitalWrite(PIN_RED, HIGH);
        delay(100);
        digitalWrite(PIN_RED, LOW);    
      }
    }
  }
  if (!displayDisabled) {
    String message = readMessageFromEEPROM();
    displayMessage(message);
    delay(t_pause);
  }
}
