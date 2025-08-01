/**
 */

#include <string.h> //needed for memcpy
#include "morse.h"

extern uint32_t t_dot;
extern uint32_t t_dash;
extern uint32_t t_space;
extern uint32_t t_pause;

extern bool radioEnabled;

extern bool followerEnabled;

extern byte message[];
extern int message_len;

extern byte commBuffer[];

// single line of message read from serial console
const int LINE_SIZE = 64;
byte line[LINE_SIZE];
int line_len;

void showInstructions() {
  Serial.println(F("In-stream modifiers for text interpretation:"));
  Serial.println(F("    _  Morse (default)"));
  Serial.println(F("  $    Hexadecimal"));
  Serial.println(F("  #    Unary"));
  Serial.println(F("  %    Chess"));
  Serial.println();
  Serial.println(F("Timing commands:"));
  Serial.println(F("  *s<dot>"));
  Serial.println(F("      changes the dot duration (speed) to <dot> ms"));
  Serial.println(F("  *p<pause>"));
  Serial.println(F("      changes the inter-message pause to <pause> ms"));
  Serial.println();
  Serial.println(F("Manual control:"));
  Serial.println(F(" ^0   Sets output GPIO low"));
  Serial.println(F(" ^1   Sets output GPIO high"));
  Serial.println(F(" ^    Resumes message output. Syncs message with master"));
  Serial.println();
  Serial.println(F("Secondary output control:"));
  Serial.println(F(" >0   Disables output 2"));
  Serial.println(F(" >1   Enables output 2"));
}

/*
 * Parse a star command, either *s<num> or *p<num>
 */
uint32_t parseIntFromLine() {
  uint32_t res = 0;
  for (int i = 2; i < line_len; i++) {
    char c = line[i];
    if (c >= '0' && c <= '9') {
      res = res * 10 + (c - '0');
    } else {
      return 0;
    }
  }
  return res;
}

void clearCommBuffer(int tokenType) {
  commBuffer[0] = tokenType;
  for (int k = 1; k < CHUNK_SIZE; k++) {
    commBuffer[k] = 0;
  }
}

void previewMessage() {
  printMessage();
  Serial.println(F("<\n"));
}

void transmitInteger(int tokenType, uint32_t value) {
  if (!radioEnabled) {
    return;
  }
  clearCommBuffer(tokenType);
  commBuffer[1] = (value >> 24) & 0xFF;
  commBuffer[2] = (value >> 16) & 0xFF;
  commBuffer[3] = (value >> 8) & 0xFF;
  commBuffer[4] = value & 0xFF;
  radio.write(commBuffer, COMM_BUFFER_SIZE);
}

void transmitMessage() {
  enableDisplay(true);
  if (!radioEnabled) {
    return;
  }
  Serial.println(F("-- Transmitting message"));
  clearCommBuffer(TOKEN_MESSAGE);
  for (int j = 0; j <= message_len; j++) { // include terminating 0
    byte b = message[j];
    int dest = (j % CHUNK_SIZE) + 1;
    commBuffer[dest] = b;
    if (0 == b || dest == CHUNK_SIZE) {
      radio.write(commBuffer, COMM_BUFFER_SIZE);
      clearCommBuffer(TOKEN_MESSAGE);
      delay(100); // allow time for receiver to process packet before sending the next
    }
  }
  // write empty chunk to signal end of transmission
  clearCommBuffer(TOKEN_MESSAGE);
  radio.write(commBuffer, COMM_BUFFER_SIZE);
}

void printLine() {
  for (int i = 0; i < line_len; i++) {
    Serial.print((char)line[i]);
  }
  Serial.println();
}

uint32_t decodeInteger() {
  uint32_t value = commBuffer[1];
  value = (value << 8) | commBuffer[2];
  value = (value << 8) | commBuffer[3];
  value = (value << 8) | commBuffer[4];
  return value;
}

/*
 * Read a line from serial console into line[]
 * 
 * sets line_len to number of characters read
 */
void readLine() {
  while (!Serial.available()) {
  }
  line[0] = 0;
  line_len = 0;
  while (Serial.available() && line_len < LINE_SIZE - 1) {
    char c = Serial.read();
    if ('\n' != c && '\r' != c) {
      line[line_len++] = c;
    }
    delay(10); // work around timing bug in Serial buffer
  }
  if (line_len == LINE_SIZE - 1) {
    Serial.println(F("-- entry truncated\n"));
  }
  line[line_len] = 0;
}

void appendLineToMessage() {
  int numBytes = min(line_len, MESSAGE_SIZE - message_len - 1); // do not include terminal 0 from line[]
  memcpy(message + message_len, line, numBytes);
  message_len += numBytes;
  message[message_len] = 0;
}

// line starts with '^'
void processManualCommand() {
  if (0 == line[1]) {
    // "^" was entered
    Serial.println(F("-- Resuming message output"));
    setOutput(LOW); // in case of previous ^1 command
    delay(t_space); // in case of previous ^1 command
    transmitMessage();
    return;
  }
  if (2 == line_len && ('0' == line[1] || '1' == line[1])) {
    byte value = line[1] - '0';
    enableDisplay(false);
    Serial.print(F("-- Turning output "));
    Serial.println(value ? "on" : "off");
    setOutput(value);
    transmitInteger(TOKEN_MANUAL, value);
  } else {
    Serial.println(F("-- Invalid manual command"));
  }
}

// line starts with '>'
void processFollowerCommand() {
  if (2 == line_len && ('0' == line[1] || '1' == line[1])) {
    byte value = line[1] - '0';
    Serial.print(F("-- "));
    Serial.print(value ? "Enabling" : "Disabling");
    Serial.println(F(" secondary output"));
    setFollow(value);
    transmitInteger(TOKEN_FOLLOWER, value);
  } else {
    Serial.println(F("-- Invalid secondary output control command"));
  }
}

void processStarCommand() {
  if ('s' == line[1]) { // speed change
    uint32_t speed = parseIntFromLine();
    if (speed > 0) {
      setSpeed(speed);
      transmitInteger(TOKEN_SPEED, t_dot);
    } else {
      Serial.println(F("-- Invalid speed"));
    }
  } else if ('p' == line[1]) { // pause change
    uint32_t pause = parseIntFromLine();
    if (pause > 0) {
      setPause(pause);
      transmitInteger(TOKEN_PAUSE, t_pause);
    } else {
      Serial.println(F("-- Invalid pause"));
    }
  } else {
    Serial.println(F("-- Invalid star command"));
  }
}

bool messageChanged = false;

void loop_XMIT() {

  if (buttonPressed()) {
    testRoutine();
  }
 
  if (!Serial.available()) {
  
    if (!messageChanged && message_len > 0) {
      displayMessage();
    }
    messageChanged = false;

  } else { // Serial available

    readLine();
    
    if (0 == line_len) {
      Serial.println(F("-- Message cleared"));
      message_len = 0;
      writeMessageToEEPROM();
      transmitMessage();
      return;
    }

    if ('^' == line[0]) {
      processManualCommand();
      return;
    }
    
    if ('>' == line[0]) {
      processFollowerCommand();
      return;
    }

    if ('*' == line[0]) {
      processStarCommand();
      return;
    }

    Serial.println(F("-- Data received on serial monitor\n"));

    messageChanged = true;
    message[0] = 0;
    message_len = 0;

    Serial.print(F("-- Append to message (max "));
    Serial.print(LINE_SIZE - 1, DEC);
    Serial.println(F(" chars per line.) Blank line commits message\n"));

    appendLineToMessage();
    previewMessage();
    
    readLine();
    while (line_len > 0 && message_len < MESSAGE_SIZE - 1) {
      appendLineToMessage(); 
      previewMessage();
      readLine();
    }
    if (message_len == MESSAGE_SIZE - 1) {
      Serial.println(F("-- Maximum message size reached"));
    }
    writeMessageToEEPROM();
    transmitMessage();
  }
}