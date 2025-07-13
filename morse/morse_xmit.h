/**
 Send Morse code to GPIO pin.

 Send text to slave via optional nRF24L01 radio.

 @author topquark22
*/

#ifndef MORSE_XMIT_H
#define MORSE_XMIT_H

#include "morse.h"

extern uint32_t t_dot;
extern uint32_t t_dash;
extern uint32_t t_space;
extern uint32_t t_pause;

extern RF24 radio;

void showInstructions();

/*
 * Parse a star command, either *s<num> or *p<num>
 */
uint32_t parseIntFromLine();

void clearCommBuffer();

void previewMessage();

void transmitInteger(int tokenType, uint32_t value);

void transmitMessage();

void printLine();

uint32_t decodeInteger();

/*
 * Read a line from serial console
 * 
 * sets line_len to number of characters read
 */
void readLine();

void appendLineToMessage();

void processStarCommand();

void loop_XMIT();

#endif
