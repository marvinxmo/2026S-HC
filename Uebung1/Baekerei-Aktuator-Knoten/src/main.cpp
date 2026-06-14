#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

const int PIN_SOLENOID = 32; 

typedef struct struct_nachricht {
  bool alarmAusloesen;
} struct_nachricht;

struct_nachricht meineDaten;

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  memcpy(&meineDaten, incomingData, sizeof(meineDaten));
  
  Serial.print("[FUNK] Signal empfangen. Alarm-Status: ");
  Serial.println(meineDaten.alarmAusloesen ? "AKTIV! 🚨" : "Inaktiv");

  // Logik für Low-Active Relais umgedreht:
  if (meineDaten.alarmAusloesen == true) {
    Serial.println("[AKTOR] Löse Solenoid aus! *KLANG* 🔔");
    digitalWrite(PIN_SOLENOID, LOW);  // LOW schaltet das Relais EIN
    delay(300);                       // 300 Millisekunden klopfen lassen
    digitalWrite(PIN_SOLENOID, HIGH); // HIGH schaltet das Relais wieder AUS
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_SOLENOID, OUTPUT);
  // Wichtig bei Low-Active: Beim Start auf HIGH setzen, damit es AUS bleibt!
  digitalWrite(PIN_SOLENOID, HIGH); 

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERROR] Fehler beim Starten von ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("[READY] Backstuben-Empfänger (Low-Active) bereit...");
}

void loop() {
  delay(1000);
}