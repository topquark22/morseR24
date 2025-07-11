/**
 * General includes
*/

#ifndef MORSE_H
#define MORSE_H

#include <Arduino.h>
#include <string.h>  // needed for memcpy
#include <RF24_config.h>
#include <nRF24L01.h>
#include <RF24.h>
#import <EEPROM.h>

#include "morse_xmit.h"
#include "morse_recv.h"

const PROGMEM int PIN_DISABLE_ = 4; // wire to GND if not using radio

const PROGMEM int PIN_XRMODE = 8; // wire to GND for recv (slave) mode; else xmit (master) mode

const PROGMEM int PIN_BUTTON_ = 6; // code key switch for manual input in test mode

const PROGMEM int PIN_OUT = 5;  // output
const PROGMEM int PIN_OUT_ = 3; // inverted output
const PROGMEM int PIN_OUT2 = 7; // optional output
const PROGMEM int PIN_RED = 2;  // LED, hemorrhoid condition

// radio output power = 2*A0 + A1
const PROGMEM int PIN_PWR2 = A0;
const PROGMEM int PIN_PWR1 = A1;

// Which RF channel to communicate on, 0-125
// default 118 = 88 + 10 + 20
const int CHANNEL_BASE = 88;
const PROGMEM int PIN_CH10 = A2; // if not wired low, add 10 to CHANNEL_BASE
const PROGMEM int PIN_CH20 = A3; // if not wired low, add 20 to CHANNEL_BASE

// Device ID setting. Must match radio and radio
const uint64_t DEVICE_ID_BASE = 0x600DFF2440LL;
const PROGMEM int PIN_ID2 = A4; // if wired low, add 0x2 to ID_BASE
const PROGMEM int PIN_ID1 = A5; // if wired low, add 0x1 to ID_BASE

// Detect boards having support for analog input on A6, A7
#if NUM_ANALOG_INPUTS >= 8 && !defined(MORSER24_NO_PWM)
  #define __USE_PWM
#endif

#ifdef __USE_PWM
  #warning Compiled with PWM support. You can use -D NO_PWM to disable it.
// analog input for PWM duty cycle. In most cases you would wire this HIGH
// (it's right next to the +5V pin), or else connect to a POT/voltage divider.
const PROGMEM int PIN_PWM = A7;
#endif

// These wirings of CE, CSN are used for integrated Nano3/nRF24l01 boards
const PROGMEM int PIN_CE = 10;
const PROGMEM int PIN_CSN = 9;

// defaults
const int t_DOT = 100;
const int t_PAUSE = 3000;

// Serial transmission rate
const int BAUD_RATE = 9600;

// special value indicating that an int in EEPROM has not been set
const int NOT_SET = -1;

// packet type tokens (first byte of commBuffer)
const int TOKEN_MANUAL = 1;
const int TOKEN_SPEED = 2;
const int TOKEN_PAUSE = 3;
const int TOKEN_MESSAGE = 4;
const int TOKEN_FOLLOWER = 5;

const int SPI_SPEED = 10000000;

// full message buffer
const int MESSAGE_SIZE = 240;

// determines location of message banks in EEPROM
const int MSG_BANK_SIZE = 0x100;

// EEPROM addresses (add msgBankAddr)
const int OFFSET_ADDR_SPEED = 0xF0;
const int OFFSET_ADDR_PAUSE = 0xF4;
const int OFFSET_ADDR_FOLLOW = 0xF8;

// commBuffer[] is used to store/receive message via radio
// Format:
//   commBuffer[0] : token indicating message type (1 byte)
//   commBuffer[1] to commBuffer[31]: data block
const int COMM_BUFFER_SIZE = 32;
const int CHUNK_SIZE = COMM_BUFFER_SIZE - 1;

/**
   Text interpretation (can using special character mid-message)
*/
enum TEXT_INTERPRETATION {
  MORSE, // '_' in stream
  HEXADECIMAL,  // '$' in stream
  UNARY, // '#' in stream
  CHESS  // '%' in stream
};

const char SW_MORSE = '_';
const char SW_HEXADECIMAL  = '$';
const char SW_UNARY = '#';
const char SW_CHESS = '%';

// Width of serial console
const int CONSOLE_WIDTH = 80;

const int EEPROM_LEN = 0x3F0; // leave room for speed, pause

void blinkRedLED(unsigned long ms);

void showSettings();

void clearMessage();

void prepareDevice();

void setupRadio();

int decodeInteger();

void printMessage();

void writeMessageToEEPROM();

void readMessageFromEEPROM();

void writeIntToEEPROM(int addr, int value);

int readIntFromEEPROM(int addr);

void writeSpeedToEEPROM();

void readSpeedFromEEPROM();

void writePauseToEEPROM();

void readPauseFromEEPROM();

void writeFollowToEEPROM();

void readFollowFromEEPROM();

#ifdef USE_PWM
int getPWM();
#endif

void setOutput(bool value);

void errExit();

void enableDisplay(bool enabled);

void displayMessage();

bool buttonPressed();

void testRoutine();

// Sets the red LED and other status
void setErrorIndicator(bool status);

/*
   Sets the duration of 1 dot in ms
*/
void setSpeed(int t_dot_ms);

void setPause(int t_pause_ms);

void setFollow(bool follow);

/*
 * Append commBuffer bytes to message buffer
 * @return number of bytes appended
 */
int decodeCommBuffer();

void testRoutine();

#endif
