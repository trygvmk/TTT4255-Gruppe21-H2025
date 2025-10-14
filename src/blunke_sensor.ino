#include <WiFiNINA.h>
#include <Arduino_LSM6DS3.h>

// 🔹 WiFi-info
const char* ssid = "TESTNETT";
const char* password = "Gjest2003";

// 🔹 Opprett webserver
WiFiServer server(80);

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

  server.begin();
  Serial.println("Webserver startet!");
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

  // --- Webserver ---
  WiFiClient client = server.available();
  if (!client) return;

  String request = client.readStringUntil('\r');
  client.flush();

  // --- Send status ---
  if (request.indexOf("/data") != -1) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println(alarmOn ? "Alarm AV" : "Alarm PÅ");
    client.stop();
    return;
  }

  // --- Reset alarm via nettsiden ---
  if (request.indexOf("/reset") != -1) {
    alarmOn = true;
    digitalWrite(alarmPin, HIGH);
    blinkCount = 0;
    Serial.println("🔄 Alarmen ble resatt via nettsiden");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Alarm resatt");
    client.stop();
    return;
  }

  // --- HTML-side ---
  String html = F(
    "<!DOCTYPE html>"
    "<html lang='no'>"
    "<head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>Alarmstatus</title>"
    "<style>"
    "body { font-family: Arial, sans-serif; background:#f8f9fa; text-align:center; padding:40px; }"
    "h1 { color:#007bff; margin-bottom:10px; }"
    "#statusBox { display:inline-block; background:white; padding:20px 40px; border-radius:15px;"
    " box-shadow:0 4px 10px rgba(0,0,0,0.1); margin-top:30px; }"
    ".statusText { font-size:1.5rem; font-weight:bold; margin:10px 0; }"
    "button { padding:12px 25px; font-size:16px; border:none; border-radius:8px; cursor:pointer; margin:10px;"
    " transition:background 0.3s; }"
    "#resetBtn { background:#dc3545; color:white; }"
    "#resetBtn:hover { background:#b02a37; }"
    "</style>"
    "<script>"
    "async function oppdater(){"
      "const res=await fetch('/data');"
      "const status=await res.text();"
      "const sElem=document.getElementById('status');"
      "sElem.innerText=status;"
      "if(status.includes('PÅ')) sElem.style.color='red';"
      "else sElem.style.color='green';"
    "}"
    "async function resetAlarm(){"
      "await fetch('/reset');"
      "await oppdater();"
      "alert('Alarmen er resatt');"
    "}"
    "setInterval(oppdater,1000);"
    "</script>"
    "</head>"
    "<body onload='oppdater()'>"
    "<h1>Blunkdeteksjon</h1>"
    "<div id='statusBox'>"
    "<p class='statusText'>Alarmstatus: <span id='status'>Laster...</span></p>"
    "<button id='resetBtn' onclick='resetAlarm()'>Reset alarm</button>"
    "</div>"
    "</body></html>"
  );

  // --- Send HTML til klient ---
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println(html);
  client.stop();
}
