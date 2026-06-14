#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

const int PIN_KUNDE = 12;   
const int PIN_BAECKER = 14; 

uint8_t empfaengerAdresse[] = {0x30, 0xC9, 0x22, 0xF2, 0x67, 0xC0};

typedef struct struct_nachricht {
  bool alarmAusloesen;
} struct_nachricht;

struct_nachricht meineDaten;
esp_now_peer_info_t peerInfo;

// ─── NEUE VARIABLEN FÜR DIE REALISTISCHE LOGIK ───────────────────
bool alarmBereitsAusgeloest = false; 
unsigned long zeitpunktLetzteBewegung = 0;
const unsigned long RUHE_ZEITSPANNE = 8000; // 8 Sekunden Ruhe, bevor ein neuer Kunde klingeln kann

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Feedback im Monitor schlanker halten
  if (meineDaten.alarmAusloesen) {
    Serial.print("[FUNK] Alarm-Befehl gesendet. Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Zustellung ✅" : "Fehler ❌");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_KUNDE, INPUT);
  pinMode(PIN_BAECKER, INPUT);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERROR] Fehler beim Starten von ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  
  memcpy(peerInfo.peer_addr, empfaengerAdresse, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("[ERROR] Empfänger konnte nicht registriert werden");
    return;
  }
  
  Serial.println("[READY] Smarter Verkaufsraum-Sender aktiv!");
}

void loop() {
  int statusKunde = digitalRead(PIN_KUNDE);
  int statusBaecker = digitalRead(PIN_BAECKER);
  unsigned long aktuelleZeit = millis();

  // 1. Wenn Bewegung vorm Laden ist, merken wir uns den Zeitstempel
  if (statusKunde == HIGH) {
    zeitpunktLetzteBewegung = aktuelleZeit;
  }

  // 2. LOGIK: Kunde betritt Laden, Bäcker fehlt UND wir haben für diesen Kunden noch NICHT geklingelt
  if (statusKunde == HIGH && statusBaecker == LOW && !alarmBereitsAusgeloest) {
    Serial.println("\n[LOGIK] 🚶 Kunde betritt den Laden! Löse EINMALIGen Alarm aus.");
    
    meineDaten.alarmAusloesen = true;
    esp_now_send(empfaengerAdresse, (uint8_t *) &meineDaten, sizeof(meineDaten));
    
    alarmBereitsAusgeloest = true; // Sperre setzen, damit es nicht dauerhaft rattert
  }

  // 3. ZURÜCKSETZEN: Wenn für die 'RUHE_ZEITSPANNE' kein Kunde mehr gesehen wurde,
  // setzen wir die Sperre zurück. Der Laden gilt als wieder "leer".
  if (alarmBereitsAusgeloest && (aktuelleZeit - zeitpunktLetzteBewegung > RUHE_ZEITSPANNE)) {
    Serial.println("[LOGIK] 🛡️ Laden ist seit einigen Sekunden leer. System bereit für nächsten Kunden.");
    alarmBereitsAusgeloest = false;
    
    // Dem Aktuator kurz Bescheid geben, dass der Zustand wieder "Inaktiv" ist
    meineDaten.alarmAusloesen = false;
    esp_now_send(empfaengerAdresse, (uint8_t *) &meineDaten, sizeof(meineDaten));
  }

  // Ganz kurze Pause (50ms), um den Prozessor zu entlasten, aber sofort auf Bewegung zu reagieren
  delay(50);
}