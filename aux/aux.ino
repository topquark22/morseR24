/**
 * Code for auxiliary Arduino B to control another Arduino A running morseR24
 */
 
// connect to pins on the other Arduino A running morseR24
const int PIN_A_D6 = 6;
const int PIN_A_RST = 2;

void setup() {
  pinMode(PIN_A_D6, OUTPUT);
  pinMode(PIN_A_RST, OUTPUT);

  // "hold down" code key switch on A to enter test mode
  digitalWrite(PIN_A_D6, LOW);

  // reset A
  digitalWrite(PIN_A_RST, LOW);
  delay(10);
  digitalWrite(PIN_A_RST, HIGH);

}

void loop() {
  // Blink A by controlling code key switch pin
  digitalWrite(PIN_A_D6, LOW);
  delay(500);
  digitalWrite(PIN_A_D6, HIGH);
  delay(500);
}
