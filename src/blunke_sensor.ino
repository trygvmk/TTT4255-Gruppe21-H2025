#include <Arduino_LSM6DS3.h>

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Start LSM6DS3 (akselerometer + gyroskop)
  if (!IMU.begin()) {
    Serial.println("Feil: Kunne ikke starte IMU!");
    while (1);
  }
  Serial.println("IMU (akselerometer og gyroskop) aktivert.");
}

void loop() {
  float ax, ay, az; // akselerometerverdier (g)
  float gx, gy, gz; // gyroskopverdier (grader per sekund)

  // Les akselerometerdata
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
  }

  // Les gyroskopdata
  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(gx, gy, gz);
  }

  // Skriv ut data
  Serial.print("Aks: ");
  Serial.print(ax); Serial.print(", ");
  Serial.print(ay); Serial.print(", ");
  Serial.print(az); Serial.print(" g");

  Serial.print(" | Gyro: ");
  Serial.print(gx); Serial.print(", ");
  Serial.print(gy); Serial.print(", ");
  Serial.print(gz); Serial.println(" °/s");

  delay(200);
}
