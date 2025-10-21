#include <WiFiNINA.h>
#include <Arduino_LSM6DS3.h>
#include <ArduinoHttpClient.h>

// 🔹 WiFi-info
const char* ssid = "TESTNETT";
const char* password = "Gjest2003";


// 🔹 Node.js server info
const char* serverAddress = "10.22.142.90"; // <-- SET THIS TO YOUR PC'S IP
int serverPort = 3000;
WiFiClient wifi;
HttpClient httpClient(wifi, serverAddress, serverPort);
bool lastSentAlarmOn = true;

// 🔹 Alarm- og sensorinnstillinger
const int alarmPin = 5;
const float threshold = 0.2;           // Grense for "blunk"
const unsigned long window = 2000;     // 2 sekunder
unsigned long blinkTimes[10];
int blinkCount = 0;
bool alarmOn = true;



void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(alarmPin, OUTPUT);
  digitalWrite(alarmPin, HIGH); // Start med alarm på

  if (!IMU.begin()) {
    Serial.println("Feil: kunne ikke starte IMU!");
    while (1);
  }

  Serial.println("Kobler til WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi tilkoblet!");
  Serial.print("IP-adresse: ");
  Serial.println(WiFi.localIP());

  // Ikke lenger webserver
}

void loop() {
  // --- Les akselerasjon ---
  float ax, ay, az;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    float magnitude = sqrt(ax * ax + ay * ay + az * az);

    static float lastMag = 1.0;
    float delta = fabs(magnitude - lastMag);
    lastMag = magnitude;

    if (delta > threshold) {
      unsigned long now = millis();
      blinkTimes[blinkCount % 10] = now;
      blinkCount++;
      Serial.println("💡 Registrert bevegelse (blunk)");
    }

    // --- Fjern gamle blunk ---
    unsigned long now = millis();
    int validCount = 0;
    for (int i = 0; i < blinkCount && i < 10; i++) {
      if (now - blinkTimes[i] <= window) validCount++;
    }

    // --- Sjekk om alarm skal slås av ---
    if (alarmOn && validCount >= 4) {
      Serial.println("🚨 Fire blunk registrert – alarm på!");
      alarmOn = false;
      digitalWrite(alarmPin, LOW);
    }
  }


  // --- Sjekk og send alarmstatus hvis endret ---
  if (lastSentAlarmOn != alarmOn) {
    String json = String("{\"alarmOn\":") + (alarmOn ? "true" : "false") + "}";
    httpClient.post("/data", "application/json", json);
    lastSentAlarmOn = alarmOn;
    Serial.print("Sendte alarmstatus til server: ");
    Serial.println(alarmOn ? "AV" : "PÅ");
  }
}
