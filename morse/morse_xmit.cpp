/**
 * functions related to transmission only
 */


#include <string.h> //needed for memcpy
#include "morse.h"

extern int t_dot;
extern int t_dash;
extern int t_space;
extern int t_pause;

extern bool radioEnabled;

extern byte message[];
extern int message_len;

extern byte commBuffer[];

// single line of message read from serial console
const int LINE_SIZE = 64;
byte line[LINE_SIZE];
int line_len;

void showInstructions() {
    Serial.print("Dot duration: "); Serial.print(t_dot); Serial.println(" ms");
    Serial.print("Pause duration: "); Serial.print(t_pause); Serial.println(" ms");
    Serial.println();
    Serial.println("Accepting input from serial console");
    Serial.println();
    Serial.println("Star commands:");
    Serial.println("  *s<dot>");
    Serial.println("    changes the dot duration (speed) to <dot> ms");
    Serial.println("  *p<pause>");
    Serial.println("    changes the inter-message pause to <pause> ms");
    Serial.println();
    Serial.println("In-stream modifiers for text interpretation:");
    Serial.println("  _: Morse (default)");
    Serial.println("  $: Hexadecimal");
    Serial.println("  #: Unary");
    Serial.println("  %: Chess");
    Serial.println();
}

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

void clearCommBuffer(int tokenType) {
  commBuffer[0] = tokenType;
  for (int k = 1; k < CHUNK_SIZE; k++) {
    commBuffer[k] = 0;
  }
}

void previewMessage() {
  printMessage();
  Serial.println("<\n");
}

void transmitInteger(int tokenType, int value) {
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
  if (!radioEnabled) {
    return;
  }
  Serial.println("-- Transmitting message");
  clearCommBuffer(TOKEN_MESSAGE);
  for (int j = 0; j <= message_len; j++) { // include terminating 0
    byte b = message[j];
    int dest = (j % CHUNK_SIZE) + 1;
    commBuffer[dest] = b;
    if (0 == b || dest == CHUNK_SIZE) {
      radio.write(commBuffer, COMM_BUFFER_SIZE);
      clearCommBuffer(TOKEN_MESSAGE);
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

int decodeInteger() {
  return (commBuffer[1] << 24) + (commBuffer[2] << 16) + (commBuffer[3] << 8) + commBuffer[4];
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
  }
  if (line_len == LINE_SIZE - 1) {
    Serial.println("-- entry truncated\n");
  }
  line[line_len] = 0;
}

void appendLineToMessage() {
  int numBytes = min(line_len, MESSAGE_SIZE - message_len - 1); // do not include terminal 0 from line[]
  memcpy(message + message_len, line, numBytes);
  message_len += numBytes;
  message[message_len] = 0;
}

void processStarCommand() {
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
  } else {
    Serial.println("-- Invalid * command");
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
      Serial.println("-- Message cleared");
      message_len = 0;
      writeMessageToEEPROM();
      transmitMessage();
      return;
    }

    if ('*' == line[0]) {
      processStarCommand();
      return;
    }

    Serial.println("-- Data received on serial monitor\n");

    messageChanged = true;
    message[0] = 0;
    message_len = 0;

    Serial.print("-- Append to message (max ");
    Serial.print(LINE_SIZE - 1, DEC);
    Serial.println(" chars per line.) Blank line commits message\n");

    appendLineToMessage();
    previewMessage();
    
    readLine();
    while (line_len > 0 && message_len < MESSAGE_SIZE - 1) {
      appendLineToMessage(); 
      previewMessage();
      readLine();
    }
    if (message_len == MESSAGE_SIZE - 1) {
      Serial.println("-- Maximum message size reached");
    }
    writeMessageToEEPROM();
    transmitMessage();
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
