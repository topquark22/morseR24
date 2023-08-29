/**
 Send Morse code to GPIO pin.

 Send text to slave via optional nRF24L01 radio.

 @author topquark22
*/

#include "morse.h"

extern RF24 radio;

/*
 * Append commBuffer bytes to message buffer
 * 
 * @return number of bytes appended
 */
int decodeCommBuffer();

void receiveMessage();

static void setup_RECV();

void loop_RECV();