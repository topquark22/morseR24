# morseR24

An Arduino sketch that converts text to Morse code, outputs to GPIO pin, and can transmit to slave devices using nRF24L01 radio.

It supports up to 16 different wireless channels, and two different device IDs.

## Hardware requirements

This project uses the Arduino Nano V3, or a compatible board with integrated nRF24L01 radio. Other boards may work, but they must have A7 analog input. So, for instance, a Uno won't work.

## Software requirements

You must install the RF24 library from TMRh20,Avamander version 1.4.7 or greater.

## Initialization

This code will work with a fresh Arduino from the factory. The EEPROM location 0 must contain the value 0xFF at first use, in order for it to initialize the timings to their default values. If something gets messed up, the best thing to do is run eeprom_reset.ino. This will clear the EEPROM to all 0xFF values. Then reupload the sketch and life is good.

## PWM wiring

The signal output pins (D5, and D3 inverted) support PWM. This is controlled by the voltage on pin A7, which is usually wired to +5V. If you leave A7 unconnected, you will get erratic results. On the Nano V3, it is easy to wire A7 to +5 because the two pins are right next to each other. In most cases that's what you will do unless you really want to use PWM.

## CE and CSN pins

The integrated boards are wired for CE=10 and CSN=9. Most resources on the Internet suggest `RF24 radio(9, 10)` or say that you can specify any pins. That's fine if you are using an external radio for some other project, but this project supports integrated boards and so you must use pin 10 for CE and pin 9 for CSN.

## Modes of operation

### Master (transmitter) mode

For master mode, leave pin D8 unconnected. The message to transmit is entered in the serial monitor console. It accepts multiple lines (as the serial input buffer can only accept 64 characters at a time,) and concatenates them together. Enter a blank line to commit the message.

The following in-line characters will cause the text to be interpreted in several different ways:

- Morse code (the default, or preceded by **_** )
- Unary (using the **#** character). For example, **#314159**
- Hexadecimal (using the **$** character). For example, **$600DF00D**
- Chess coordinates (using the **%** character). For example, **%e2e4**

You can change these in-line modifiers mid-text.

The timings can be changed using the following commands, for example:

- ***speed 100** changes the dot length to 100 ms
- ***pause 3000** changes the inter-message pause to 3000 ms

The command will be transmitted to the slave. Issuing one of these commands will interrupt and restart any broadcast in progress.

### Slave (receiver) mode

For slave mode, wire pin D8 to ground.

The serial monitor will display the characters as they are transmitted, as well as any speed and pause timing changes.

## Error conditions

The red LED (pin D2) indicates the following:

It will blink once for 1/10 second upon initiation to indicate that the radio is operational.

Fast blink: Radio is not connected properly. You can use this code in master mode without a radio, if you just want to control something from the GPIO pin. But in that case you need to wire pin D4 to ground to disable the radio.

Solid red can mean one of two things:

-  If red LED lights immediately, that means you are configured as a receiver and the radio is disabled. This configuration is unsupported.
    
-  If it happens during transmission of a message, the PIN_RED will light up on the slave to indicate that an invalid packet was received. The problem should be investigated and the slave reset.

## Test mode

There are two ways to enter test mode:

1.  Wire pin D7 to ground. When the Arduino is reset, the output will blink continuously every 1 second. If you hold down the button during the reset, then the button will act as a code key for manual input.

2.  In normal operation (D7 not wired to ground), you can enter test mode by holding down the button until the next loop starts. In this case, the button will act as a code key for manual input. To get out of test mode, reset the Arduino.

## Example circuits

### External device control

This example uses an integrated Nano V3 + nRF24L01 board. Here, the output D5 drives the gate of a 2N7000 MOSFET in open drain configuration, that can then be used to control an external circuit.

![MOSFET board](mosfet-board.jpg)

Similarly, the inverted output D3 could be used to drive a P-channel MOSFET such as the IRF9540.

### Tone generator

Here, an LM555 timer in astable mode generates a tone that is sent to a mono audio jack. The pin D5 output signal is sent to pin 4 (active low reset) of the 555 to turn the tone on and off. This configuration uses a regular Nano V3 and external radio (not shown) connected to the SPI bus. This circuit can be used in master mode (with or without radio) or slave mode.

![Tone generator](audio-board.jpg)
