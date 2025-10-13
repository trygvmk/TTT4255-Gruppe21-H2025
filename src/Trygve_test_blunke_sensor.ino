#include <Arduino_LSM6DS3.h>

const float accelThreshold = 0.5;   // Lower threshold for subtle eyebrow movement
const float gyroThreshold = 30.0;        // Lower threshold for subtle rotation
const int window = 1000;
const int debounceTime = 50;

unsigned long blinkTimes[10];
int blinkCount = 0;
bool alarmOn = true;
unsigned long lastBlinkTime = 0;

// Low-pass filter for gravity removal
float ax_filtered = 0, ay_filtered = 0, az_filtered = 0;
const float alpha = 0.8; // Filter coefficient (higher = more filtering)

// Previous acceleration for jerk calculation
float ax_prev = 0, ay_prev = 0, az_prev = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!IMU.begin()) {
    Serial.println("Feil: kunne ikke starte IMU!");
    while (1);
  }

  Serial.println("Starter blunkdeteksjon (hybrid accel+gyro)...");
  Serial.println("Blunk raskt 4 ganger på 1 sekund for å slå av alarm");
  
  // Initialize filters
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax_filtered, ay_filtered, az_filtered);
    ax_prev = ax_filtered;
    ay_prev = ay_filtered;
    az_prev = az_filtered;
  }
}

void loop() {
  float ax, ay, az, gx, gy, gz;
  bool blinkDetected = false;

  // Read both accelerometer and gyroscope
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    IMU.readGyroscope(gx, gy, gz);

    // Low-pass filter to estimate gravity component
    ax_filtered = alpha * ax_filtered + (1 - alpha) * ax;
    ay_filtered = alpha * ay_filtered + (1 - alpha) * ay;
    az_filtered = alpha * az_filtered + (1 - alpha) * az;

    // High-pass filter: subtract gravity to get dynamic acceleration
    float ax_dynamic = ax - ax_filtered;
    float ay_dynamic = ay - ay_filtered;
    float az_dynamic = az - az_filtered;

    // Calculate acceleration (rate of change of acceleration)
    float accel_x = fabs(ax_dynamic - ax_prev);
    float accel_y = fabs(ay_dynamic - ay_prev);
    float accel_z = fabs(az_dynamic - az_prev);
    float accelMagnitude = sqrt(accel_x * accel_x + accel_y * accel_y + accel_z * accel_z);

    // Calculate gyroscope magnitude
    float gyroMagnitude = sqrt(gx * gx + gy * gy + gz * gz);

    // Store current acceleration for next iteration
    ax_prev = ax_dynamic;
    ay_prev = ay_dynamic;
    az_prev = az_dynamic;

    // Detect blink using EITHER high acceleration OR high rotation
    if (accelMagnitude > accelThreshold || gyroMagnitude > gyroThreshold) {
      blinkDetected = true;
      
      unsigned long now = millis();
      
      // Debounce: only register if enough time has passed since last blink
      if (now - lastBlinkTime > debounceTime) {
        blinkTimes[blinkCount % 10] = now;
        blinkCount++;
        lastBlinkTime = now;
        
        Serial.print("💡 Blunk! (accel: ");
        Serial.print(accelMagnitude);
        Serial.print(" g/s, gyro: ");
        Serial.print(gyroMagnitude);
        Serial.println(" °/s)");
      }
    }

    // Count valid blinks within the time window
    unsigned long now = millis();
    int validCount = 0;
    for (int i = 0; i < 10; i++) {
      if (now - blinkTimes[i] <= window) {
        validCount++;
      }
    }

    // Check if alarm should be turned off
    if (alarmOn && validCount >= 4) {
      Serial.println("🚨 FIRE BLUNK REGISTRERT PÅ 1 SEKUND – ALARM AV!");
      Serial.println("========================================");
      alarmOn = false;
      
      // TODO: Send Bluetooth alert when alarm is triggered
      // sendBluetoothAlert();
    }
  }

  delay(10); // Fast sampling for better detection
}

// ============================================================================
// BLUETOOTH COMMUNICATION SECTION (COMMENTED OUT)
// ============================================================================
// Uncomment and configure this section when ready to implement Bluetooth
// 
// Common Arduino Bluetooth modules:
// - HC-05 / HC-06 (Classic Bluetooth)
// - HM-10 (Bluetooth Low Energy / BLE)
// - Built-in Bluetooth on Arduino Nano 33 IoT / Arduino Nano RP2040 Connect
//
// ============================================================================

/*
// Include appropriate Bluetooth library based on your module:
// For HC-05/HC-06 (uses Serial communication):
// #include <SoftwareSerial.h>
// SoftwareSerial BTSerial(10, 11); // RX, TX pins

// For BLE (e.g., HM-10 or built-in BLE):
// #include <ArduinoBLE.h>

// Bluetooth setup - add this to setup() function
void setupBluetooth() {
  // For HC-05/HC-06:
  // BTSerial.begin(9600); // Most HC modules default to 9600 baud
  // Serial.println("Bluetooth initialisert");
  
  // For BLE:
  // if (!BLE.begin()) {
  //   Serial.println("Starting BLE failed!");
  //   while (1);
  // }
  // BLE.setLocalName("BlinkSensor");
  // BLE.setAdvertisedService(serviceUUID);
  // BLE.advertise();
  // Serial.println("BLE initialisert");
}

// Send alert when 4 blinks detected
void sendBluetoothAlert() {
  // For HC-05/HC-06 (simple serial communication):
  // BTSerial.println("ALARM_OFF");
  // BTSerial.print("Blink count: ");
  // BTSerial.println(blinkCount);
  
  // For BLE (write to characteristic):
  // blinkCharacteristic.writeValue("ALARM_OFF");
  
  Serial.println("Bluetooth alarm sendt!");
}

// Send periodic status updates (call this in loop if needed)
void sendBluetoothStatus() {
  // Send current sensor data over Bluetooth
  // BTSerial.print("Accel: ");
  // BTSerial.print(accelMagnitude);
  // BTSerial.print(", Gyro: ");
  // BTSerial.println(gyroMagnitude);
}

// Receive commands from Bluetooth (optional)
void receiveBluetoothCommands() {
  // For HC-05/HC-06:
  // if (BTSerial.available()) {
  //   String command = BTSerial.readStringUntil('\n');
  //   command.trim();
  //   
  //   if (command == "RESET") {
  //     alarmOn = true;
  //     blinkCount = 0;
  //     Serial.println("Alarm reset via Bluetooth");
  //   }
  //   else if (command == "STATUS") {
  //     BTSerial.print("Alarm: ");
  //     BTSerial.println(alarmOn ? "ON" : "OFF");
  //     BTSerial.print("Blinks: ");
  //     BTSerial.println(blinkCount);
  //   }
  // }
}

// Example BLE Service and Characteristic setup (for BLE modules)
// BLEService blinkService("19B10000-E8F2-537E-4F6C-D104768A1214");
// BLEStringCharacteristic blinkCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20);
// BLEByteCharacteristic commandCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLEWrite);
*/

// ============================================================================
// INSTRUCTIONS FOR BLUETOOTH INTEGRATION:
// ============================================================================
// 1. Uncomment the appropriate library and code section above
// 2. Add setupBluetooth() call in setup() function
// 3. Uncomment sendBluetoothAlert() call in the alarm detection section
// 4. Optional: Add receiveBluetoothCommands() in loop() for two-way communication
// 5. For HC-05/HC-06: Connect TX->RX and RX->TX between Arduino and BT module
// 6. For BLE: Follow your specific BLE module's wiring instructions
// ============================================================================
