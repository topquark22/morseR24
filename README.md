# morseR24

Send Morse code to GPIO pin and optional RF24 radio

This version transmits the entire message to the receiver. The receiver stores it in its EEPROM.

## Initialization

This code will work with a fresh Arduino from the factory. The EEPROM location 0 must contain the value 0xFF initially, in order for it to initialize the speed and pause to their initial default values. If something gets messed up, the best thing to do is run eeprom_reset.ino. This will clear the EEPROM to all 0xFF values. Then reupload the sketch and life is good.

## Hardware

The hardware for this project is based around the Arduino Nano3 and/or a board with an integrated radio.

In fact, it looks like the Uno doesn't even have a pin A7, so you may have to use a Nano.

## CE and CSN

The important point for the integrated board is that you must use CE=10 and CSN=9. Most resources on the Internet suggest radio(9, 10) or some other pins. That's fine if you are using a Uno or something else, but it took me some time to figure this out. I had to follow the traces on the PCB and compare the pinout with the nRF24L01 chip on the integrated board to see that they actually have to be reversed. I couldn't find any explanation of this anywhere, so here it is.

## Error conditions

The red LED (pin D2) indicates the following:

It will blink once for 1/10 second upon initiation to indicate that the radio is operational.

Fast blink: Radio is not connected properly. You can use this code without a radio, if you just want to control something from the GPIO pin. But in that case you need to wire pin D4 to ground to disable the radio.

Solid red: This can mean one of two things:

-  If it happens immediately, that means you are configured as a receiver and the radio is disabled. This is obviously an unsuported configuration because you can't do anything.
    
-  If it happens during transmission of a message, the PIN_RED will light up on the slave to indicate a bad checksum of the data packet. This can be cleared by retransmitting the data from the master.

## Test mode

There are two ways to enter test mode:

1.  Wire pin D7 to ground. When the Arduino is reset, the output will blink continuously each 1 second. This is useful if you are in a different room, for instance.

If you hold down the button during the reset, then the button will act as a code key for manual input.

2.  In normal operation (D7 not wired to ground), you can enter test mode by holding down the button until the next loop starts. In this case, the button will act as a code key for manual input. To get out of test mode, you have to reset the Arduino.

## Hardware example

Here is just one way to wire up the Arduino. In this circuit, the output drives the gate of a 2N7000 MOSFET which can then be used to control an external circuit.

![MOSFET board](img/mosfet-board.jpg)
