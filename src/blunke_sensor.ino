#include <WiFiNINA.h>
#include <Arduino_LSM6DS3.h>

// 🔹 WiFi-info
const char* ssid = "TESTNETT";
const char* password = "Gjest2003";


// 🔹 Node.js server info
int HTTP_PORT = 3000;
String HTTP_METHOD = "POST";      // or "POST"
char HOST_NAME[] = "172.20.10.4"; // hostname of web server:
String PATH_NAME = "/data";

WiFiClient client;

bool lastSentAlarmOn = false;

// 🔹 Alarm- og sensorinnstillinger
const int alarmPin = 5;
const float threshold = 0.2;       // Grense for "blunk"
const unsigned long window = 2000; // 2 sekunder
unsigned long blinkTimes[10];
int blinkCount = 0;
bool alarmOn = false;

String queryString = String("?alarmOn=") + String(alarmOn);

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;

  if (!IMU.begin())
  {
    Serial.println("Feil: kunne ikke starte IMU!");
    while (1)
      ;
  }

  Serial.println("Kobler til WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi tilkoblet!");
  Serial.print("IP-adresse: ");
  Serial.println(WiFi.localIP());

  // Ikke lenger webserver
}

void loop()
{
  // --- Les akselerasjon ---
  float ax, ay, az;
  if (IMU.accelerationAvailable())
  {
    IMU.readAcceleration(ax, ay, az);
    float magnitude = sqrt(ax * ax + ay * ay + az * az);

    static float lastMag = 1.0;
    float delta = fabs(magnitude - lastMag);
    lastMag = magnitude;

    if (delta > threshold)
    {
      unsigned long now = millis();
      blinkTimes[blinkCount % 10] = now;
      blinkCount++;
      Serial.println("💡 Registrert bevegelse (blunk)");
    }

    // --- Fjern gamle blunk ---
    unsigned long now = millis();
    int validCount = 0;
    for (int i = 0; i < blinkCount && i < 10; i++)
    {
      if (now - blinkTimes[i] <= window)
        validCount++;
    }

    // --- Sjekk om alarm skal slås på ---
    if (!alarmOn && validCount >= 4)
    {
      Serial.println("🚨 Fire blunk registrert – alarm på!");
      alarmOn = true;
    }
  }

  // --- Sjekk og send alarmstatus hvis endret ---
  if (lastSentAlarmOn != alarmOn)
  {
    if (client.connect(HOST_NAME, HTTP_PORT))
    {
      client.println("POST " + PATH_NAME + " HTTP/1.1");
      client.println("Host: " + String(HOST_NAME));
      client.println("Connection: close");
      client.println(); // end HTTP header

      // send HTTP body
      client.println(queryString);
    }
  }
}
