/**
 Send Morse code to GPIO pin.

 Send text to slave via optional nRF24L01 radio.

 @author topquark22
*/

#ifndef R_A
#define R_A

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

extern byte payload[];

/*
 * Append payload bytes to message buffer
 * 
 * @return number of bytes appended
 */
int decodePayload() {
  if (payload[0] != TOKEN_MESSAGE) {
    Serial.print("Expected message; got payload of type: ");
    Serial.println(payload[0]);
    errExit();
  }
  int i;
  for (i = 0; i < BLOCK_SIZE && payload[i + 1] > 0 && message_len < MESSAGE_SIZE - 1; i++) {
    message[message_len++] = payload[i + 1];
  }
  message[message_len] = 0;
  return i;
}

void receiveMessage() {
  Serial.println("-- Receiving message");
  // first block already in buffer
  message[0] = 0;
  message_len = 0;
  int bytesAppended = decodePayload();
  while (bytesAppended > 0) {
    while (!radio.available()) {
      // wait for next packet
    }
    radio.read(payload, PAYLOAD_SIZE);
    bytesAppended = decodePayload();
  }
  writeMessageToEEPROM();
}

bool displayEnabled = true;

void loop_RECV() {
  if (radio.available()) {
    digitalWrite(PIN_RED, LOW);
    radio.read(payload, PAYLOAD_SIZE); // Read data from the nRF24L01
    if (TOKEN_MESSAGE == payload[0]) {
     receiveMessage();
      displayEnabled = true;
    } else if (TOKEN_TEST == payload[0]) { // special case manual transmission
      setOutput(payload[4]);
      displayEnabled = false;
    } else if (TOKEN_SPEED == payload[0]) { // speed was transmitted
      setSpeed(decodeInteger());
    } else if (TOKEN_PAUSE == payload[0]) { // pause was transmitted
      setPause(decodeInteger());
    } else { // invalid token in payload[0]
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

#endif