#include <Arduino_LSM6DS3.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>

// WiFi network credentials
char ssid[] = "Get-2G-DC87F1";       // your network SSID (name)
char pass[] = "CopanoRickey100%BestUma";   // your network password

// Node.js server address and port
const char serverAddress[] = "192.168.0.155";  // Replace with your server IP
int port = 3000;

// HTTP client using WiFiNINA (use WiFiSSLClient for HTTPS)
WiFiClient wifi;
HttpClient client(wifi, serverAddress, port);

const float threshold = 0.2;        // example motion threshold
unsigned long blinkTimes[10];
int blinkCount = 0;
const unsigned long window = 2000;        // time window in ms for blinks
bool alarmOn = false;
bool prevAlarmOn = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);  // wait for serial monitor

  // Connect to WiFi network
  Serial.print("Connecting to WiFi");
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  float ax, ay, az;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    float magnitude = sqrt(ax*ax + ay*ay + az*az);
    static float lastMag = 1.0;
    float delta = fabs(magnitude - lastMag);
    lastMag = magnitude;

    if (delta > threshold) {
      // Register a motion (blink)
      unsigned long now = millis();
      blinkTimes[blinkCount % 10] = now;
      blinkCount++;
      Serial.println("💡 Registrert bevegelse (blunk)");
    }

    // Count blinks within the time window
    unsigned long now = millis();
    int validCount = 0;
    for (int i = 0; i < blinkCount && i < 10; i++) {
      if (now - blinkTimes[i] <= window) validCount++;
    }

    // Turn alarm on if enough blinks detected
    if (!alarmOn && validCount >= 4) {
      Serial.println("🚨 Fire blunk registrert – alarm på!");
      alarmOn = true;
    }
  }

  // If alarm state changed, send HTTP POST with JSON
  if (alarmOn != prevAlarmOn) {
    // Build JSON string
    String contentType = "application/json";
    String postData = String("{\"alarmOn\":") + (alarmOn ? "true" : "false") + "}";

    // Send POST request to /data endpoint
    client.post("/data", contentType, postData);

    // Read and print response (optional)
    int statusCode = client.responseStatusCode();
    String response = client.responseBody();
    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);

    prevAlarmOn = alarmOn;
  }

  delay(100);
}
