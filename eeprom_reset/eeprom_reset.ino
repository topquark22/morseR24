#include <EEPROM.h>

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 1024; i++) {
    EEPROM.write(i, 0xFF);
  }
  exit(0);
}
