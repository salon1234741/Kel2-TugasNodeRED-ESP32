#include "EspMQTTClient.h"
#include <DHT.h>

// ===== GANTI SESUAI KELOMPOK, RUANGAN, DAN JARINGAN =====
#define KELOMPOK  "klp2"            // nomor kelompok
#define ROOM      "ruang3"          // ruang1 .. ruang4  (E7: hanya ini yang diganti)
#define WIFI_SSID "HALO DEK"
#define WIFI_PASS "11223344"
#define BROKER    "broker.mqttdashboard.com"

// ===== TOPIC & CLIENT ID dibentuk dari KELOMPOK + ROOM (E2, E7) =====
#define PREFIX    "pens/" KELOMPOK "/" ROOM
const char* T_TEMP   = PREFIX "/temperature";
const char* T_HUM    = PREFIX "/humidity";
const char* T_L1     = PREFIX "/lamp1";
const char* T_L2     = PREFIX "/lamp2";
const char* T_STATUS = PREFIX "/status";

// ===== PIN =====
#define DHTPIN  18
#define DHTTYPE DHT11
const int L1_PIN = 25;
const int L2_PIN = 26;

DHT dht(DHTPIN, DHTTYPE);

EspMQTTClient client(
  WIFI_SSID, WIFI_PASS,
  BROKER,
  "", "",                           // user, pass (kosong untuk broker publik)
  "esp32-" KELOMPOK "-" ROOM,       // E2: Client ID unik
  1883);

// E6: hanya "1" atau "0" yang diproses
void setLamp(int pin, const String& payload) {
  if (payload == "1") {
    digitalWrite(pin, HIGH); // Nyalakan LED
  } else if (payload == "0") {
    digitalWrite(pin, LOW);  // Matikan LED
  }
  // payload lain diabaikan
}

void setup() {
  Serial.begin(115200);
  pinMode(L1_PIN, OUTPUT);
  pinMode(L2_PIN, OUTPUT);
  dht.begin();

  client.enableDebuggingMessages();
  client.setKeepAlive(15);
  
  // E3: daftarkan LWT (Last Will and Testament)
  client.enableLastWillMessage(T_STATUS, "offline", true);
}

// Dipanggil library setiap kali (ulang) tersambung ke broker
void onConnectionEstablished() {
  Serial.println("MQTT Connection established.");

  // E4: subscribe T_L1 -> panggil setLamp(L1_PIN, payload)
  client.subscribe(T_L1, [](const String& payload) { 
    setLamp(L1_PIN, payload);
  }, 1);

  // E4: subscribe T_L2 -> panggil setLamp(L2_PIN, payload)
  client.subscribe(T_L2, [](const String& payload) { 
    setLamp(L2_PIN, payload);
  }, 1);

  // E4: publish "online" (retain = true) ke T_STATUS
  client.publish(T_STATUS, "online", true);
}

unsigned long lastPublishTime = 0;

void loop() {
  client.loop();  // E1: Wi-Fi, MQTT, dan reconnect diurus di sini

  // E5: setiap 5000 ms (millis, bukan delay):
  if (millis() - lastPublishTime >= 5000) {
    lastPublishTime = millis();
    
    // baca DHT11
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    // kalau !isnan (artinya sensor berhasil terbaca), publish data
    if (!isnan(t) && !isnan(h)) {
      client.publish(T_TEMP, String(t, 1));
      client.publish(T_HUM, String(h, 1));
    } else {
      Serial.println("Gagal membaca sensor DHT!");
    }
  }
}
