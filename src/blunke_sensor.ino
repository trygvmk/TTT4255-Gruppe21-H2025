#include <Arduino_LSM6DS3.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>

// WiFi network credentials
char ssid[] = "Network name"; // your network SSID (name)
char pass[] = "Password";     // your network password

// Node.js server address and port
const char serverAddress[] = "172.20.10.4"; // Replace with your server IP
int port = 3000;

WiFiClient wifi;
HttpClient client(wifi, serverAddress, port);

const float threshold = 0.2;
unsigned long blinkTimes[10];
int blinkCount = 0;
const unsigned long window = 2000;
bool alarmOn = false;
bool prevAlarmOn = false;

unsigned long lastFetch = 0;
const unsigned long fetchIntervalAlarm = 5 * 1000;

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;

  if (!IMU.begin())
  {
    Serial.println("Failed to initialize IMU!");
    while (1)
      ;
  }

  Serial.print("Connecting to WiFi");
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED)
  {
    status = WiFi.begin(ssid, pass);
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
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

    // Count blinks within the time window
    unsigned long now = millis();
    int validCount = 0;
    for (int i = 0; i < blinkCount && i < 10; i++)
    {
      if (now - blinkTimes[i] <= window)
        validCount++;
    }

    // Turn alarm on if enough blinks detected
    if (!alarmOn && validCount >= 4)
    {
      Serial.println("🚨 Fire blunk registrert – alarm på!");
      alarmOn = true;
    }
  }

  // --- Send POST immediately on state change ---
  if (alarmOn != prevAlarmOn)
  {
    String contentType = "application/json";
    String postData = String("{\"alarmOn\":") + (alarmOn ? "true" : "false") + "}";
    client.post("/data", contentType, postData);

    int statusCode = client.responseStatusCode();
    String response = client.responseBody();
    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);

    prevAlarmOn = alarmOn;
  }

  // --- Poll server only if alarmOn is true ---
  if (alarmOn)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - lastFetch >= fetchIntervalAlarm)
    {
      lastFetch = currentMillis;

      client.get("/data");
      int getStatus = client.responseStatusCode();
      String getResponse = client.responseBody();
      if (getStatus == 200)
      {
        bool serverAlarm = getResponse.indexOf("true") >= 0;
        if (!serverAlarm && alarmOn)
        {
          alarmOn = false;
          prevAlarmOn = false;
          Serial.println("🔄 Alarm acknowledged on server, reset Arduino alarm");
        }
      }
    }
  }

  // --- Short delay to avoid overwhelming IMU ---
  delay(100);
}