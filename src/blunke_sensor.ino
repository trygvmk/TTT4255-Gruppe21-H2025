#include <Arduino.h>
void setup() {
    // put your setup code here, to run once:
        pinMode(LED_BUILTIN, OUTPUT);
    }

void loop() {
    // put your main code here, to run repeatedly:
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("PÅ");
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("AV");
        delay(500);
    }