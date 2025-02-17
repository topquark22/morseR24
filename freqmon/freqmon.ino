#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(10, 9); // CE = 10, CSN = 9

void setup() {
    Serial.begin(9600);
    radio.begin();
    radio.setAutoAck(false); // Disable auto acknowledgment
    radio.startListening(); // Put radio in receive mode
}

void loop() {
    for (int channel = 0; channel <= 125; channel++) {
        radio.setChannel(channel);
        delayMicroseconds(128); // Give time to settle
        uint8_t noise = radio.testCarrier(); // Checks if there's a signal
        Serial.print("Channel "); Serial.print(channel);
        Serial.print(": ");
        Serial.println(noise ? "Interference detected" : "Clear");
    }
    delay(2000); // Scan every 2 seconds
}