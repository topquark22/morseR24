/**
 Send Morse code to GPIO pin.

 Send text to slave via optional nRF24L01 radio.

 @author topquark22
*/

#include "morse.h"
#include "morse_recv.h"

extern int t_dot;
extern int t_dash;
extern int t_space;
extern int t_pause;

extern RF24 radio;

extern bool radioEnabled;

extern bool testMode;

extern byte message[];
extern int message_len;

extern byte commBuffer[];

/*
 * Append commBuffer bytes to message buffer
 * 
 * @return number of bytes appended
 */
int decodeCommBuffer() {
  if (commBuffer[0] != TOKEN_MESSAGE) {
    Serial.print("Expected message; got commBuffer of type: ");
    Serial.println(commBuffer[0]);
    errExit();
  }
  int i;
  for (i = 0; i < CHUNK_SIZE && commBuffer[i + 1] > 0 && message_len < MESSAGE_SIZE - 1; i++) {
    message[message_len++] = commBuffer[i + 1];
  }
  message[message_len] = 0;
  return i;
}

void receiveMessage() {
  Serial.println("-- Receiving message");
  // first block already in buffer
  message[0] = 0;
  message_len = 0;
  int bytesAppended = decodeCommBuffer();
  while (bytesAppended > 0) {
    while (!radio.available()) {
      // wait for next packet
    }
    radio.read(commBuffer, COMM_BUFFER_SIZE);
    bytesAppended = decodeCommBuffer();
  }
  writeMessageToEEPROM();
}

bool displayEnabled = true;

void loop_RECV() {
  if (radio.available()) {
    digitalWrite(PIN_RED, LOW);
    radio.read(commBuffer, COMM_BUFFER_SIZE); // Read data from the nRF24L01
    if (TOKEN_MESSAGE == commBuffer[0]) {
     receiveMessage();
      displayEnabled = true;
    } else if (TOKEN_TEST == commBuffer[0]) { // special case manual transmission
      setOutput(commBuffer[4]);
      displayEnabled = false;
    } else if (TOKEN_SPEED == commBuffer[0]) { // speed was transmitted
      setSpeed(decodeInteger());
    } else if (TOKEN_PAUSE == commBuffer[0]) { // pause was transmitted
      setPause(decodeInteger());
    } else { // invalid token in commBuffer[0]
      Serial.println("Invalid packet received");
      digitalWrite(PIN_RED, HIGH);
      delay(100);
      exit(1);
    }
  }
  if (displayEnabled) {
    displayMessage();
    delay(t_pause);
  } else {
    delay(10);
  }
}
