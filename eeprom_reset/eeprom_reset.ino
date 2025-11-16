#include <EEPROM.h>

void setup() {
  Serial.begin(9600);
  Serial.println(F("Resetting EEPROM to factory state..."));
  for (uint16_t i = 0; i < 0x400; i++) {
    EEPROM.write(i, 0xFF);
  }
  Serial.println(F("Done."));
  exit(0);
}
