/**
 Send Morse code to GPIO pin.

 Send text to slave via optional nRF24L01 radio.

 @author topquark22
*/

#ifndef X_A
#define X_A

#include "morse.h"

extern int t_dot;
extern int t_dash;
extern int t_space;
extern int t_pause;

extern RF24 radio;

void showInstructions();

/*
 * Parse a star command, either *s<num> or *p<num>
 */
int parseIntFromLine();

void clearPayload();

void previewMessage();

void transmitInteger(int tokenType, int value);

void transmitMessage();

void printLine();

int decodeInteger();

void transmitInteger(int tokenType, int value);

/*
 * Read a line from serial console
 * 
 * sets line_len to number of characters read
 */
void readLine();

void appendLineToMessage();

void processStarCommand();

void loop_XMIT();

void testRoutine();

#endif