const int MODE_PIN = 2;

void setup() {
  pinMode(MODE_PIN, INPUT_PULLUP);  // Button connected to GND when pressed
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(4, OUTPUT);
}

void loop() {
  int currentVal = digitalRead(MODE_PIN);

  // Set outputs based on button state every time
  if (currentVal == HIGH) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(4, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(4, LOW);
  }

  delay(10); // Small debounce, optional
}
