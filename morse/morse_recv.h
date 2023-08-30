/**
 Send Morse code to GPIO pin.

 Send text to slave via optional nRF24L01 radio.

 @author topquark22
*/

#ifndef MORSE_RECV_H
#define MORSE_RECV_H

#include "morse.h"

extern RF24 radio;

void receiveMessage();

void loop_RECV();

#endif
