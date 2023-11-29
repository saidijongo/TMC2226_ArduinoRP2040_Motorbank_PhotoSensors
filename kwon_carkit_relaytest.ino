const int totalLEDs = 7;
int ledPins[totalLEDs];

void setup() {
  for (int pin = 0; pin < totalLEDs; pin++) {
    ledPins[pin] = 5 + pin;
    pinMode(ledPins[pin], OUTPUT);
  }
}

void loop() {
  // Turn on all LEDs
  for (int pin = 0; pin < totalLEDs; pin++) {
    digitalWrite(ledPins[pin], HIGH);
  }
  delay(100000);  // Delay with all LEDs on

  // Turn off all LEDs
  for (int pin = 0; pin < totalLEDs; pin++) {
    digitalWrite(ledPins[pin], LOW);
  }
  delay(2000);  // Delay with all LEDs off
}
