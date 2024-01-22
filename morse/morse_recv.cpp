/**
 Send Morse code to GPIO pin.

 Send text to slave via optional nRF24L01 radio.

 @author topquark22
*/

#include "morse.h"
#include "morse_recv.h"

extern RF24 radio;

extern bool radioEnabled;

extern bool followerEnabled;

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
    Serial.print(F("Expected message packet; got packet of type: "));
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
  Serial.println(F("-- Receiving message"));
  // first block already in buffer
  clearMessage();
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

void loop_RECV() {
  if (radio.available()) {
    digitalWrite(PIN_RED, LOW);
    radio.read(commBuffer, COMM_BUFFER_SIZE); // Read data from the nRF24L01
    if (TOKEN_MESSAGE == commBuffer[0]) {
      enableDisplay(true);
      receiveMessage();
    } else if (TOKEN_MANUAL == commBuffer[0]) { // special case manual transmission
      enableDisplay(false);
      setOutput(commBuffer[4]);
    } else if (TOKEN_SPEED == commBuffer[0]) { // speed was transmitted
      setSpeed(decodeInteger());
    } else if (TOKEN_PAUSE == commBuffer[0]) { // pause was transmitted
      setPause(decodeInteger());
    } else if (TOKEN_FOLLOWER == commBuffer[0]) { // follower control command received
      followerEnabled = decodeInteger();
    } else { // invalid token in commBuffer[0]
      Serial.println(F("Invalid packet received"));
      digitalWrite(PIN_RED, HIGH);
      delay(100);
      exit(1);
    }
  }
  displayMessage();
}
