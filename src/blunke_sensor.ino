#include <Arduino_LSM6DS3.h>
#include <WiFiNINA.h>

const char* ssid = "TESTNETT";
const char* password = "Gjest2003";

WiFiServer server(80);      // <-- Denne linjen manglet!

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

  Serial.println("Kobler til WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi tilkoblet!");
  Serial.print("IP-adresse: ");
  Serial.println(WiFi.localIP());   // ⬅️ Her får du IP-adressen
  server.begin();
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

 // --- Webserver ---
  WiFiClient client = server.available();
  if (!client) return;

  String request = client.readStringUntil('\r');
  client.flush();

  if (request.indexOf("/data") != -1) {
    // Send status til nettsiden
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println(alarmOn ? "Alarm PÅ" : "Alarm AV");
    client.stop();
    return;
  }

  // --- HTML-side ---
  String html = F(
    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>Alarmstatus</title>"
    "<script>"
    "async function oppdater(){"
      "const res=await fetch('/data');"
      "const status=await res.text();"
      "document.getElementById('status').innerText=status;"
    "}"
    "setInterval(oppdater,1000);"
    "</script></head><body>"
    "<h1>Blunkdeteksjon</h1>"
    "<p>Alarmstatus: <span id='status'>Laster...</span></p>"
    "</body></html>"
  );

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println(html);
  client.stop();
}