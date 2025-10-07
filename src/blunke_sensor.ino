#include <Arduino_LSM6DS3.h>

const int alarmPin = 5;      // Kobles til LED eller buzzer
const float threshold = .4; // G-grense for "blunk" (juster etter behov)
const unsigned long window = 1000; // 2 sekunder

unsigned long blinkTimes[10];
int blinkCount = 0;
bool alarmOn = true;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(alarmPin, OUTPUT);
  digitalWrite(alarmPin, HIGH); // Start med alarm på

  if (!IMU.begin()) {
    Serial.println("Feil: kunne ikke starte IMU!");
    while (1);
  }

  Serial.println("Starter blunkdeteksjon...");
}

void loop() {
  float ax, ay, az;

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);

    float magnitude = sqrt(ax * ax + ay * ay + az * az);

    // Hvis rask endring i akselerasjon -> registrer blunk
    static float lastMag = 1.0;
    float delta = fabs(magnitude - lastMag);
    lastMag = magnitude;

    if (delta > threshold) {
      unsigned long now = millis();
      blinkTimes[blinkCount % 10] = now;
      blinkCount++;
      Serial.println("💡 Registrert bevegelse (blunk)");
    }

    // Fjern gamle blunk (eldre enn 2 sekunder)
    unsigned long now = millis();
    int validCount = 0;
    for (int i = 0; i < blinkCount && i < 10; i++) {
      if (now - blinkTimes[i] <= window) validCount++;
    }

    // Sjekk om alarm skal slås av
    if (alarmOn && validCount >= 4) {
      Serial.println("🚨 Fire blunk registrert på 2 sekunder – alarm av!");
      alarmOn = false;
      digitalWrite(alarmPin, LOW);
    }
  }

  delay(50); // juster for hastighet
}
