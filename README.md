# morseR24

An Arduino sketch that converts text to Morse code or other formats, outputs to GPIO pin, and can transmit to slave devices using an nRF24L01 radio.

It supports up to 8 wireless channels, and 2 selectable device IDs.

## Hardware requirements

This project was developed using the Arduino Nano V3, and a compatible board with integrated nRF24L01 radio. Some other Arduinos may work, but they must have analog input pin A7. So, for instance, a Uno won't work.

## Software requirements

You must install the RF24 library from TMRh20,Avamander version 1.4.7 or greater.

## Initialization

Upload **morse.ino**. The sketch will work initially with Arduino factory EEPROM settings. If the stored message or timing settings are incorrect, upload **eeprom_reset.ino** to reset the EEPROM. Then reupload **morse.ino**.

## CE and CSN pins

If using an external radio, wire CE to pin 10 and CSN to pin 9. This project supports integrated boards and that's what they use.

## Circuit considerations

The output pin is D5 and the inverted output is D3.

### PWM wiring

The output pins support PWM. This is controlled by the voltage on pin A7, which is usually wired to +5V. If you leave A7 unconnected, you will get erratic results. On the Nano V3, it is easy to wire A7 to +5 because the two pins are right next to each other. In most cases that's what you will do unless you really want to use PWM.

### Indicator LED

Optionally connect a red LED from pin D2 via a current-limiting resistor to ground. This is used to indicate error and operating conditions (see below).

### Pushbutton switch

Optionally connect pin D6 to ground via a normally-open switch.

## Indicator signals

The red LED will blink once for 1/10 second if the radio is operational.

Fast blink: Radio is enabled but not connected properly. If you just want to control something with GPIO, you can operate in master mode without a radio. But in that case you must wire pin D4 to ground to disable the radio.

Solid red can mean one of two things:

-  If the red LED lights immediately, the circuit is configured as a receiver and the radio is disabled. This configuration is not supported.
    
-  If the red LED lights during transmission of a message, an invalid packet was received. Ensure that the transmitter and the receiver are running compatible versions and that there is good radio communication. The problem should be investigated and the slave reset.

## Configuration

### Test mode

There are two ways to enter test mode:

1.  Wire pin D7 to ground. When the Arduino is reset, the output will blink continuously every 1 second. If you hold down the button during the reset, then the button will act as a code key for manual input.

2.  In normal operation (D7 not wired to ground), you can enter test mode by holding down the button until the next loop starts. In this case, the button will act as a code key for manual input. To get out of test mode, reset the Arduino.

### Channel setting

The nRF24L01 supports wireless channels 0-125. The morse24 supports 8 channels, from 48 to 118 in increments of 10. The channel is specified using jumpers on pins A2-A4. If none of these are wired to ground, the channel defaults to 118. See the code for more details.

### Device ID setting

The morse24 supports two device IDs, selected by a jumper on pin A5. See the code for more details.

### Power setting

The radio power is set by jumpers on pins A0, A1. The default is MAX. See the code for more details.

## Modes of operation

### Master (transmitter) mode

For master mode, leave pin D8 unconnected.

The message to transmit is entered in the serial monitor console. It accepts multiple lines (as the serial input buffer can only accept 64 characters at a time) and concatenates them together. Enter a blank line to commit the message.

The following in-line characters will cause the text to be interpreted in several different ways:

- Morse code (the default, or preceded by **_** )
- Unary (using the **#** character). For example, **#314159**
- Hexadecimal (using the **$** character). For example, **$600DF00D**
- Chess coordinates (using the **%** character). For example, **%e2e4 e7e5**

You can change these in-line modifiers mid-text.

The timings can be changed using the following commands, for example:

- ***speed 100** changes the dot length to 100 ms
- ***pause 3000** changes the inter-message pause to 3000 ms

The command will be transmitted to the slave. Issuing one of these commands will interrupt and restart any broadcast in progress.

### Slave (receiver) mode

For slave mode, wire pin D8 to ground.

The serial monitor will display the characters as they are transmitted, as well as any speed and pause timing changes.

## Example circuits

### External device control

This example uses an integrated Nano V3 + nRF24L01 board. Here, the output D5 drives the gate of a 2N7000 MOSFET in open drain configuration, that can then be used to control an external circuit.

![MOSFET board](mosfet-board.jpg)

Similarly, the inverted output D3 could be used to drive a P-channel MOSFET such as the IRF9540.

### Tone generator

Here, an LM555 timer in astable mode generates a tone that is sent to a mono audio jack. The pin D5 output signal is sent to pin 4 (active low reset) of the LM555 to modulate the tone. This configuration uses a regular Nano V3 and external radio (not shown) connected to the SPI bus. This circuit can be used in master mode (with or without radio) or slave mode.

![Tone generator](audio-board.jpg)
