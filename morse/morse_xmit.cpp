/**
 * functions related to transmission only
 */

#include <Arduino.h> //needed for Serial.println
#include <string.h> //needed for memcpy
#include "morse.h"

extern int t_dot;
extern int t_dash;
extern int t_space;
extern int t_pause;

extern bool radioEnabled;

extern byte message[];
extern int message_len;

extern byte payload[];

// single line of message read from serial console
const int LINE_SIZE = 64;
byte line[LINE_SIZE];
int line_len;

/*
 * Parse a star command, either *s<num> or *p<num>
 */
int parseIntFromLine() {
  int res = 0;
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

void clearPayload() {
  payload[0] = TOKEN_MESSAGE;
  for (int k = 1; k < PAYLOAD_SIZE; k++) {
    payload[k] = 0;
  }
}

void previewMessage() {
  for (int i = 0; i < message_len; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println("<");
}

void transmitInteger(int tokenType, int value) {
  if (!radioEnabled) {
    return;
  }
  clearPayload();
  payload[0] = tokenType;
  payload[1] = (value >> 24) & 0xFF;
  payload[2] = (value >> 16) & 0xFF;
  payload[3] = (value >> 8) & 0xFF;
  payload[4] = value & 0xFF;
  radio.write(payload, PAYLOAD_SIZE);
}

void transmitMessage() {
  if (!radioEnabled) {
    return;
  }
  Serial.println("-- Transmitting message");
  previewMessage(); // DEBUG
  clearPayload();
  for (int j = 0; j < message_len; j++) {
    byte c = message[j];
    payload[(j % BLOCK_SIZE) + 1] = c;
    if (0 == c || (j % BLOCK_SIZE) + 1 == BLOCK_SIZE) {
      radio.write(payload, PAYLOAD_SIZE);
      clearPayload();
    }
  }
  // write empty packet to signal end of message
  clearPayload();
  radio.write(payload, PAYLOAD_SIZE);
}

void printLine() {
  for (int i = 0; i < line_len; i++) {
    Serial.print((char)line[i]);
  }
  Serial.println();
}

int decodeInteger() {
  return (payload[1] << 24) + (payload[2] << 16) + (payload[3] << 8) + payload[4];
}

/*
 * Read a line from serial console
 * 
 * sets line_len to number of characters read
 */
void readLine() {
  while (!Serial.available()) {
    // wait for some entry
    delay(10);
  }
  line[0] = 0;
  line_len = 0;
  while (Serial.available() && line_len < LINE_SIZE - 1) {
    char c = Serial.read();
    if ('\n' != c && '\r' != c) {
      Serial.print("DEBUG readLine() got character "); Serial.println(c);
      line[line_len++] = c;
    } else {
      Serial.println("DEBUG readLine() got CR or LF");
    }
  }
  line[line_len] = 0;
  Serial.print("DEBUG readLine set line_len="); Serial.println(line_len);
}

void appendLineToMessage() {
  Serial.print("DEBUG start appendLineToMessage(), line="); printLine();
  int numBytes = min(line_len + 1, MESSAGE_SIZE - message_len);
  memcpy(message + message_len, line, numBytes);
  message_len += line_len;
  message[message_len] = 0;
  Serial.print("DEBUG: message_len="); Serial.println(message_len);
  Serial.print("DEBUG end appendLineToMessage(), message="); previewMessage();
}

void processStarCommand() {
  if (line_len > 1) {
    if ('s' == line[1]) { // speed change
      int speed = parseIntFromLine();
      if (speed > 0) {
        setSpeed(speed);
        transmitInteger(TOKEN_SPEED, t_dot);
      } else {
        Serial.println("-- Invalid speed");
      }
    } else if ('p' == line[1]) { // pause change
      int pause = parseIntFromLine();
      if (pause > 0) {
        setPause(pause);
        transmitInteger(TOKEN_PAUSE, t_dot);
      } else {
        Serial.println("-- Invalid pause");
      }
    }
  } else {
    Serial.println("-- Invalid * command");
  }
}

bool messageChanged = false;

void loop_XMIT() {
  
  if (!Serial.available()) {

    if (!messageChanged) {
        if (message_len > 0) {
        displayMessage();
        Serial.println();
        delay(t_pause);
      } else {
        delay(10);
      }
    }

  } else { // Serial.available()

    readLine();
    
    if (line_len > 0 && '*' == line[0]) {
      processStarCommand();
      return;
    }

    messageChanged = true;
    message[0] = 0;
    message_len = 0;

    if (0 == line_len) {
      Serial.println("-- Message cleared");
      transmitMessage();
      writeMessageToEEPROM();
      return;
    }
 
    Serial.println("-- Append to message (max 64 chars per line. Blank line commits message)");
    Serial.println();

    appendLineToMessage();
    previewMessage();
    
    readLine();
    while (line_len > 0) {
      appendLineToMessage(); 
      previewMessage();
      readLine();
    }
    
    messageChanged = true;
    transmitMessage();
    writeMessageToEEPROM();
  }
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