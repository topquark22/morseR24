#include <EEPROM.h>

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 0x400; i++) {
    byte c = EEPROM.read(i);
    Serial.print(i, HEX);
    Serial.print("\t");
    Serial.print("0x");
    Serial.print(c < 0x10 ? "0" : "");
    Serial.print(c, HEX);
    Serial.print("\t");
    Serial.println((char)c);
  }
  delay(100);
  exit(0);
}
