/*  Umweltmonitor-Basismodul — Firmware ENTWURF v1 (Patch-Set Zweitmeinung eingearbeitet)
 *  Board: ESP32-D0WD NodeMCU (30 Pin).  Stand: 2026-06-25.
 *
 *  ⚠ KOMPILIERT SAUBER (Core esp32 3.3.10, arduino-cli, exit 0, 2026-06-25) — aber NICHT GEFLASHT,
 *     NICHT auf Hardware getestet. Verifikation gegen ../TESTPLAN.md.
 *     TODOs: Secrets (NVS), LE-Root-CA, Kalibrierwerte, EN-Polarität, MiCS-Teiler.
 *     Fixes: RAIN_D Pull-up · NTP-Sync abgewartet · Secrets-Guard · JSON-Puffer+snprintf ·
 *            TSL2591 echter Lux · MiCS-Warmup 3 min · Watchdog core 2.x/3.x · dewpoint_c (additiv).
 *
 *  Sensorik (alle Breakout-Module, Pull-ups intern):
 *    Analog ADC1 : Bodenfeuchte IO36 · Wasserstand IO39 · Schall IO34 ·
 *                  Gas IO35 (über 10k/10k-Teiler) · UV IO32 · Regen-Intensität IO33
 *    Digital     : Regen DO IO27 (in) · MiCS-EN IO26 (out)
 *    I2C Bus0(21/22): AHT20 0x38 · BMP280 0x76 · TCS34725 0x29 · TMP102 0x48
 *    I2C Bus1(18/19): TSL2591 0x29
 *
 *  Bibliotheken: Adafruit AHTX0, Adafruit BMP280, Adafruit TCS34725,
 *                Adafruit TSL2591, Adafruit BusIO/Unified Sensor.
 *                (TMP102 wird direkt per I2C gelesen, keine Extra-Lib.)
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Preferences.h>
#include <time.h>
#include <math.h>
#include "mbedtls/md.h"
#include "esp_task_wdt.h"
#include <ESPmDNS.h>

#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_TCS34725.h>
#include <Adafruit_TSL2591.h>

// ---------- Pins ----------
#define PIN_SOIL   36   // HW-390 AOUT  (ADC1)
#define PIN_WATER  39   // HW-038 S     (ADC1)
#define PIN_NOISE  34   // MAX9814 OUT  (ADC1, schnelles Sampling)
#define PIN_GAS    35   // MiCS-5524 AO (ADC1, über 10k/10k-Teiler)
#define PIN_UV     32   // GUVA OUT     (ADC1)
#define PIN_RAIN_A 33   // Raindrop AO  (ADC1, Intensität)
#define PIN_RAIN_D 27   // Raindrop DO  (digital)
#define PIN_MICS_EN 26  // MiCS Enable  (out, HIGH = an)
#define I2C0_SDA 21
#define I2C0_SCL 22
#define I2C1_SDA 18
#define I2C1_SCL 19
#define TMP102_ADDR 0x48

// ---------- Konfig ----------
const uint32_t SEND_INTERVAL_MS = 60UL * 1000;   // ~1 min
const uint32_t MICS_WARMUP_MS   = 180UL * 1000;  // MOS-Sensor 3-5 min aufwärmen (sonst erste Messungen ungültig)
const uint8_t  WDT_TIMEOUT_S    = 30;
const char*    DEVICE_ID        = "umweltmonitor_basis_01";
const uint8_t  SCHEMA_VERSION   = 1;

// ---------- Globals ----------
#define I2C0 Wire        // globale Core-Instanz Bus 0 (direkt; KEINE eigene TwoWire(0), KEINE Referenz)
#define I2C1 Wire1       // globale Core-Instanz Bus 1
Adafruit_AHTX0   aht;
Adafruit_BMP280  bmp(&I2C0);
Adafruit_TCS34725 tcs(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_1X);
Adafruit_TSL2591 tsl(2591);
Preferences prefs;
WiFiServer sseServer(80);   // Live-Stream ueber WLAN (Server-Sent Events)
WiFiClient sseClient;       // genau ein Panel-Client

String wifiSsid, wifiPass, n8nUrl, hmacKey, caCert;
uint32_t seqCounter = 0;
uint32_t bootMillis = 0;
bool ahtOK = false, bmpOK = false, tcsOK = false, tslOK = false, tmpOK = false;  // Sensor-Presence
volatile bool wifiDisconnected = false;   // asynchron von WiFi-Event-Handler gesetzt
bool mdnsActive = false;                  // true wenn mDNS läuft (wird bei Disconnect/Reconnect zurückgesetzt)

// ISRG Root X1 (Let's Encrypt), exakt von https://letsencrypt.org/certs/isrgrootx1.pem geladen 2026-06-25.
const char* LE_ROOT_CA =
"-----BEGIN CERTIFICATE-----\n"
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n"
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n"
"WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n"
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n"
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n"
"h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n"
"0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n"
"A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n"
"T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n"
"B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n"
"B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n"
"KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n"
"OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n"
"jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n"
"qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n"
"rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n"
"HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n"
"hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n"
"ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n"
"3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n"
"NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n"
"ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n"
"TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n"
"jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n"
"oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n"
"4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n"
"mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n"
"emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n"
"-----END CERTIFICATE-----\n";

// ---------- Helpers ----------
int avgRead(uint8_t pin, int n = 16) {
  long s = 0; for (int i = 0; i < n; i++) s += analogRead(pin);
  return (int)(s / n);
}

float readNoiseRel() {                 // Single-Pass AC-RMS: Mittel + Varianz aus DENSELBEN Samples
  const int N = 1500;
  double sum = 0, sumsq = 0;
  for (int i = 0; i < N; i++) { int v = analogRead(PIN_NOISE); sum += v; sumsq += (double)v * v; }
  double mean = sum / N;
  double var = sumsq / N - mean * mean;
  return sqrt(fmax(0.0, var));          // Clamp unmittelbar in der Wurzel; relativ, KEIN dB SPL
}

float readTmp102(TwoWire& bus) {
  bus.beginTransmission(TMP102_ADDR); bus.write(0x00);
  if (bus.endTransmission() != 0) return NAN;
  if (bus.requestFrom(TMP102_ADDR, 2) < 2) return NAN;
  int16_t raw = (bus.read() << 8) | bus.read();
  raw >>= 4;                           // 12 Bit
  return raw * 0.0625f;                // °C
}

float dewPointC(float tC, float rh) {  // Magnus; invariant gegen ESP32-Eigenerwärmung (e kürzt sich heraus)
  if (isnan(tC) || isnan(rh) || rh <= 0) return NAN;
  const float b = 17.625f, c = 243.04f;
  float gamma = logf(rh / 100.0f) + (b * tC) / (c + tC);
  return (c * gamma) / (b - gamma);
}

String hmacSha256Hex(const String& msg, const String& key) {
  byte out[32];
  mbedtls_md_context_t ctx; mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)key.c_str(), key.length());
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)msg.c_str(), msg.length());
  mbedtls_md_hmac_finish(&ctx, out); mbedtls_md_free(&ctx);
  char hex[65]; for (int i = 0; i < 32; i++) sprintf(hex + i * 2, "%02x", out[i]);
  return String(hex);
}

bool timeSynced() { return time(nullptr) > 1700000000; }  // > 2023-11 ⇒ NTP lieferte echte Zeit

String isoTimestamp() {
  if (!timeSynced()) return String("unsynced");           // ehrlicher Marker statt 1970
  time_t now = time(nullptr); struct tm t; gmtime_r(&now, &t);
  char buf[25]; strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &t);
  return String(buf);
}

// Zahl als JSON-Token: "null" bei NAN, sonst per fmt formatiert. Komma bleibt im Aufrufer/Format-String.
void numTok(char* dst, size_t cap, float v, const char* fmt) {
  if (isnan(v)) snprintf(dst, cap, "null");
  else          snprintf(dst, cap, fmt, v);
}

void loadSecrets() {                   // einmalig per Provisioning in NVS schreiben
  prefs.begin("um", true);
  wifiSsid = prefs.getString("ssid", "");
  wifiPass = prefs.getString("pass", "");
  n8nUrl   = prefs.getString("url",  "");
  hmacKey  = prefs.getString("hmac", "");
  caCert   = prefs.getString("ca",   LE_ROOT_CA);
  seqCounter = prefs.getUInt("seq", 0);
  prefs.end();
}

void saveSeq() { prefs.begin("um", false); prefs.putUInt("seq", seqCounter); prefs.end(); }

bool connectWifi() {
  // Nicht-blockierend: startet WiFi.begin() und kehrt sofort zurück.
  // AutoReconnect + Event-Handler übernehmen den eigentlichen Verbindungsaufbau.
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  return false;   // Verbindung noch nicht hergestellt — AutoReconnect arbeitet
}

// WiFi-Event-Handler: reagiert auf Verbindungsereignisse asynchron.
// Loggt Connect/Disconnect + Grund; setzt wifiDisconnected-Flag für Loop-Check.
void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print(F("WLAN-IP: ")); Serial.println(WiFi.localIP());
      Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
      wifiDisconnected = false;
      mdnsActive = false;   // löst mDNS-Neustart im Health-Check aus (neue IP möglich)
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
      wifiDisconnected = true;
      mdnsActive = false;
      uint8_t reason = info.wifi_sta_disconnected.reason;
      Serial.printf("WLAN-WEG (Grund %u): ", reason);
      if      (reason == 2)  Serial.println(F("AP zu weit / schwach"));
      else if (reason == 3)  Serial.println(F("WiFi-ID deauth"));
      else if (reason == 8)  Serial.println(F("AP nicht gefunden"));
      else if (reason == 15) Serial.println(F("4-Way-Handshake fehlgeschlagen"));
      else                   Serial.println(F("unbekannt"));
      break;
    }
    default: break;
  }
}

// I2C-Presence per ACK (KEIN dev.begin() -> verhindert Re-begin-Crash bei absentem Geraet).
bool i2cPresent(TwoWire& bus, uint8_t addr) {
  bus.beginTransmission(addr);
  return bus.endTransmission() == 0;
}

// WLAN-Live-Stream (SSE): neuen Browser-Client annehmen + SSE-Header senden.
void sseAccept() {
  WiFiClient c = sseServer.accept();
  if (c) {
    if (sseClient && sseClient.connected()) sseClient.stop();   // nur ein Client
    sseClient = c;
    delay(2);
    while (sseClient.available()) sseClient.read();             // HTTP-Request verwerfen
    sseClient.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nCache-Control: no-cache\r\nAccess-Control-Allow-Origin: *\r\nConnection: keep-alive\r\n\r\n"));
  }
}
void sseSend(const char* line) {
  if (sseClient && sseClient.connected()) { sseClient.print("data: "); sseClient.print(line); sseClient.print("\n\n"); }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200); delay(300);
  Serial.println(F("\nUmweltmonitor-Basismodul v1 (Entwurf)"));

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_config_t wdt_cfg = { .timeout_ms = (uint32_t)WDT_TIMEOUT_S * 1000, .idle_core_mask = 0, .trigger_panic = true };
  esp_task_wdt_reconfigure(&wdt_cfg);   // Core 3.x: TWDT ist schon vom Core initialisiert -> reconfigure statt init
#else
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
#endif
  esp_task_wdt_add(NULL);
  Serial.println(F("CK1: WDT konfiguriert")); Serial.flush();

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);      // ~0..3,1 V
  pinMode(PIN_RAIN_D, INPUT_PULLUP);   // LM393 open-collector floatet sonst -> Phantom-Regen
  pinMode(PIN_MICS_EN, OUTPUT);
  digitalWrite(PIN_MICS_EN, HIGH);     // MiCS dauerhaft an (Netzbetrieb) — TODO: Polarität prüfen

  // EIN begin() pro Bus (von uns). Danach NUR vorhandene Geraete per dev.begin() initialisieren:
  // dev.begin() auf ein absentes Geraet ruft intern Wire.begin() nach fehlgeschlagenem detect() erneut
  // -> Core-3.3.10 freeWireBuffer-Assert/Boot-Loop. Das Presence-Gate verhindert das systematisch.
  I2C0.begin(I2C0_SDA, I2C0_SCL, 100000);
  I2C1.begin(I2C1_SDA, I2C1_SCL, 100000);
  Serial.println(F("CK2: I2C-Busse begonnen")); Serial.flush();

  ahtOK = i2cPresent(I2C0, 0x38) && aht.begin(&I2C0);
  bmpOK = i2cPresent(I2C0, 0x77) && bmp.begin(0x77);   // Modul real auf 0x77 (Doku sagte 0x76)
  tcsOK = i2cPresent(I2C0, 0x29) && tcs.begin(0x29, &I2C0);
  tslOK = i2cPresent(I2C1, 0x29) && tsl.begin(&I2C1);
  tmpOK = i2cPresent(I2C0, TMP102_ADDR);   // TMP102 wird direkt gelesen, kein Lib-begin noetig
  if (!ahtOK) Serial.println(F("WARN: AHT20 (0x38) nicht aktiv"));
  if (!bmpOK) Serial.println(F("WARN: BMP280 (0x76) nicht aktiv"));
  if (!tcsOK) Serial.println(F("WARN: TCS34725 (0x29) nicht aktiv"));
  if (!tslOK) Serial.println(F("WARN: TSL2591 (0x29/Bus1) nicht aktiv"));
  if (!tmpOK) Serial.println(F("WARN: TMP102 (0x48) nicht aktiv"));
  Serial.println(F("CK3: Sensoren gescannt")); Serial.flush();

  loadSecrets();

  // WiFi-Härtung: AutoReconnect VOR begin() setzen (sonst wirkt es erst nach nächstem Reboot).
  // Modem-Sleep AUS → stabiler SSE-Server, weniger Drops/Stottern.
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);

  // Event-Handler installieren (vor connectWifi, damit erster Connect geloggt wird).
  WiFi.onEvent(WiFiEvent);

  // Initialer Connect (kurz blockierend für NTP + SSE, max 15 s).
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  Serial.print(F("WLAN-Verbindung zu ")); Serial.print(wifiSsid);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(250); esp_task_wdt_reset(); Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F(" verbunden, IP: ")); Serial.println(WiFi.localIP());
    Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
    sseServer.begin();
    mdnsActive = MDNS.begin("umweltmonitor");
    if (mdnsActive) Serial.println(F("mDNS aktiv: http://umweltmonitor.local/"));
  } else {
    Serial.println(F(" WARN: Erstverbindung fehlgeschlagen — retry im Loop"));
  }

  configTime(0, 0, "pool.ntp.org");    // UTC; n8n kann auch selbst timestampen
  // auf NTP-Sync warten (max ~10 s), sonst sendet der erste Zyklus eine 1970-Zeit
  uint32_t tns = millis();
  while (!timeSynced() && millis() - tns < 10000) { delay(200); esp_task_wdt_reset(); }
  Serial.println(timeSynced() ? F("NTP: synchronisiert") : F("NTP: noch nicht synchron (timestamp=unsynced)"));

  bootMillis = millis();
}

// ---------- Messzyklus ----------
String buildPayload() {
  // Sensoren lesen
  int   soil  = avgRead(PIN_SOIL);
  int   water = avgRead(PIN_WATER);
  int   gas   = avgRead(PIN_GAS);
  int   uv    = avgRead(PIN_UV);
  int   rainA = avgRead(PIN_RAIN_A);
  bool  rain  = (digitalRead(PIN_RAIN_D) == LOW);   // LM393: LOW = nass (Modul-abhängig, prüfen)
  float noise = readNoiseRel();
  bool  micsReady = (millis() - bootMillis) > MICS_WARMUP_MS;

  // Nur vorhandene Sensoren lesen; fehlende bleiben NAN -> JSON null via numTok.
  float airTemp = NAN, airHum = NAN, press = NAN, lux = NAN, soilTemp = NAN;
  uint16_t r = 0, g = 0, b = 0, c = 0;
  if (ahtOK) { sensors_event_t hum, tmp; aht.getEvent(&hum, &tmp); airTemp = tmp.temperature; airHum = hum.relative_humidity; }
  if (bmpOK) press = bmp.readPressure() / 100.0f;   // hPa
  if (tcsOK) tcs.getRawData(&r, &g, &b, &c);
  if (tslOK) { uint32_t lum = tsl.getFullLuminosity(); lux = tsl.calculateLux(lum & 0xFFFF, lum >> 16); }
  if (tmpOK) soilTemp = readTmp102(I2C0);
  float dewp = dewPointC(airTemp, airHum);          // NAN bei fehlendem AHT -> null

  // NAN-sichere Zahl-Tokens: fehlender Sensor -> "null" statt "nan" (sonst ungültiges JSON).
  // Kommata bleiben im Format-String -> Trennzeichen stimmen auch im null-Fall.
  char tSoil[16], tAir[16], tHum[16], tDew[16], tPress[16], tLux[16];
  numTok(tSoil,  sizeof(tSoil),  soilTemp, "%.2f");
  numTok(tAir,   sizeof(tAir),   airTemp,  "%.2f");
  numTok(tHum,   sizeof(tHum),   airHum,   "%.1f");
  numTok(tDew,   sizeof(tDew),   dewp,     "%.2f");
  numTok(tPress, sizeof(tPress), press,    "%.1f");
  numTok(tLux,   sizeof(tLux),   lux,      "%.1f");

  char buf[1024];
  int n = snprintf(buf, sizeof(buf),
    "{\"device_id\":\"%s\",\"schema_version\":%u,\"timestamp\":\"%s\",\"seq\":%lu,"
    "\"soil_moisture_raw\":%d,\"soil_temp_c\":%s,\"water_level_raw\":%d,"
    "\"rain\":%s,\"rain_intensity_raw\":%d,"
    "\"air_temp_c\":%s,\"humidity_pct\":%s,\"dewpoint_c\":%s,\"pressure_hpa\":%s,"
    "\"uv_raw\":%d,\"lux\":%s,\"color_rgb\":[%u,%u,%u],"
    "\"noise_rel\":%.1f,\"gas_raw\":%s}",
    DEVICE_ID, SCHEMA_VERSION, isoTimestamp().c_str(), seqCounter,
    soil, tSoil, water,
    rain ? "true" : "false", rainA,
    tAir, tHum, tDew, tPress,
    uv, tLux, r, g, b,
    noise, micsReady ? String(gas).c_str() : "null");
  if (n < 0 || n >= (int)sizeof(buf)) {              // Truncation -> ungültiges JSON nicht senden
    Serial.println(F("FEHLER: JSON-Puffer zu klein (Truncation)."));
    return String();
  }
  return String(buf);
}

bool sendPayload(const String& body) {
  if (wifiSsid == "" || n8nUrl == "" || hmacKey == "") {
    Serial.println(F("FEHLER: NVS-Secrets fehlen (ssid/url/hmac) - sende NICHT (kein Signieren mit leerem Key)."));
    return false;
  }
  if (body == "") return false;                          // JSON-Truncation -> nicht senden
  if (WiFi.status() != WL_CONNECTED) return false;       // Health-Check in loop() reconnectet
  WiFiClientSecure client;
  client.setCACert(caCert.c_str());        // KEIN setInsecure(): Zertifikat wird geprüft
  HTTPClient http;
  if (!http.begin(client, n8nUrl)) return false;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Signature", hmacSha256Hex(body, hmacKey));  // HMAC über den exakten Body (inkl. seq+timestamp)
  int code = http.POST(body);
  http.end();
  Serial.printf("POST -> %d\n", code);
  return code >= 200 && code < 300;
}

void loop() {
  esp_task_wdt_reset();

  // WiFi-Health-Check (nicht-blockierend, alle 30 s oder bei asynchronem Disconnect-Flag)
  static uint32_t lastWifiCheck = 0;
  if (millis() - lastWifiCheck >= 30000 || wifiDisconnected) {
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("WLAN-Wiederverbindung..."));
      if (connectWifi()) {
        // SSE-Server und mDNS nach Reconnect neu starten (neue IP möglich)
        if (!sseServer) sseServer.begin();
        if (mdnsActive) { MDNS.end(); }
        if (MDNS.begin("umweltmonitor")) {
          mdnsActive = true;
          Serial.println(F("mDNS neu registriert"));
        } else {
          mdnsActive = false;
          Serial.println(F("mDNS-Neustart fehlgeschlagen"));
        }
        Serial.print(F("WLAN-IP: ")); Serial.println(WiFi.localIP());
      }
    }
  }

  sseAccept();                       // neue WLAN-Stream-Clients annehmen
  static uint32_t last = 0, lastNoise = 0, lastLight = 0;
  uint32_t now = millis();

  // Echtzeit-Lärm ~5 Hz: "N:<rms>" (Serial + WLAN-SSE; NICHT im POST -> kein Sheet-Spam).
  if (now - lastNoise >= 200) {
    lastNoise = now;
    char nb[24]; snprintf(nb, sizeof(nb), "N:%.1f", readNoiseRel());
    Serial.println(nb); sseSend(nb);
  }

  // Echtzeit-Licht ~2 Hz: "L:<lux>,<uv_raw>,<r>,<g>,<b>" (lux=-1 falls kein TSL).
  if (now - lastLight >= 500) {
    lastLight = now;
    float lux = -1; uint16_t r = 0, g = 0, b = 0, c = 0;
    if (tslOK) { uint32_t lum = tsl.getFullLuminosity(); lux = tsl.calculateLux(lum & 0xFFFF, lum >> 16); }
    if (tcsOK) tcs.getRawData(&r, &g, &b, &c);
    char lb[48]; snprintf(lb, sizeof(lb), "L:%.1f,%d,%u,%u,%u", lux, avgRead(PIN_UV), r, g, b);
    Serial.println(lb); sseSend(lb);
  }

  if (now - last >= SEND_INTERVAL_MS || last == 0) {
    last = now;
    String body = buildPayload();
    Serial.println(body); sseSend(body.c_str());
    if (sendPayload(body)) { seqCounter++; saveSeq(); }   // seq nur bei Erfolg hoch (monoton, Replay-Schutz)
  }
  delay(20);
}
