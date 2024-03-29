/**
 Send Morse code to GPIO pin.

 Send text to slave via optional nRF24L01 radio.

 @author topquark22
*/

#include "morse.h"
#include "morse_recv.h"

const int MAX_RECV_TIME = 1000;

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

int messageRecvTimeout;

void receiveMessage() {
  Serial.println(F("-- Receiving message"));
  // first block already in buffer
  clearMessage();
  int bytesAppended = decodeCommBuffer();
  while (bytesAppended > 0) {
    messageRecvTimeout = 0;
    while (!radio.available() && messageRecvTimeout < MAX_RECV_TIME) {
      // wait for next packet
      delay(10);
      messageRecvTimeout += 10;
    }
    if (messageRecvTimeout >= MAX_RECV_TIME) {
      Serial.println(F("Partial message timeout error"));
      blinkRedLED(1000);
      return;
    }
    radio.read(commBuffer, COMM_BUFFER_SIZE);
    bytesAppended = decodeCommBuffer();
  }
  writeMessageToEEPROM();
}

void loop_RECV() {
  if (radio.available()) {
    setErrorIndicator(false);
    radio.read(commBuffer, COMM_BUFFER_SIZE); // Read data from the nRF24L01
    switch (commBuffer[0]) {
      case TOKEN_MESSAGE: {
        enableDisplay(true);
        receiveMessage();
        break;
      }
      case TOKEN_MANUAL: { // special case manual transmission
        enableDisplay(false);
        setOutput(commBuffer[4]);
        break;
      }
      case TOKEN_SPEED: { // speed was transmitted
        setSpeed(decodeInteger());
        break;
      }
      case TOKEN_PAUSE: { // pause was transmitted
        setPause(decodeInteger());
        break;
      }
      case TOKEN_FOLLOWER: { // follower control command received
        setFollow(decodeInteger());
        break;
      }
      default: { // invalid token in commBuffer[0]
        Serial.println(F("Invalid packet received"));
        setErrorIndicator(true);
      }
    }
  }
  displayMessage();
}