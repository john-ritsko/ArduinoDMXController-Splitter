#include <DMXSerial.h>
#include <DmxSimple.h>

int CHANNEL_COUNT;
const int MODE_PIN = 4;
int oldVal;
int newVal;

void setup() {
  pinMode(MODE_PIN, INPUT_PULLUP);  // if using button to pull LOW
  pinMode(LED_BUILTIN, OUTPUT);  // ← REQUIRED for LED to work

  newVal = digitalRead(MODE_PIN);

  // Set initial channel count based on input
  if (newVal == LOW) {
    CHANNEL_COUNT = 128; 
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    CHANNEL_COUNT = 256;
    digitalWrite(LED_BUILTIN, HIGH);
  }

  oldVal = newVal;

  // Initialize DMX
  DMXSerial.init(DMXReceiver);
  DmxSimple.usePin(3);
  DmxSimple.maxChannel(CHANNEL_COUNT);
}

void loop() {
  newVal = digitalRead(MODE_PIN);

  // Update channel count and reconfigure DMX if mode changed
  if (newVal != oldVal) {
    if (newVal == LOW) {
      digitalWrite(LED_BUILTIN, LOW);
      CHANNEL_COUNT = 128;
    } else {
      digitalWrite(LED_BUILTIN, HIGH);
      CHANNEL_COUNT = 256;
    }

    DmxSimple.maxChannel(CHANNEL_COUNT);  // Update output range
    oldVal = newVal;
  }

  // Mirror DMX input to output
  for (int i = 1; i <= CHANNEL_COUNT; i++) {
    int inputVal = DMXSerial.read(i);
    DmxSimple.write(i, inputVal);
  }
}
