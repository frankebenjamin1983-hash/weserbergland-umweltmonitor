# Zweitmeinung: DIY-Umweltmonitor ESP32 — Istzustand

Du bist ein unabhängiger technischer Reviewer. Ich brauche eine ehrliche Zweitmeinung zu
meinem Projekt — Schaltplan, Firmware und geplante Auswertungs-Roadmap. Keine Lobhudelei,
direkte Kritik wo nötig. Antworte auf Deutsch.

---

## Hardware-Istzustand

**MCU:** ESP32-D0WD NodeMCU 30-Pin (WROOM-32), Netzbetrieb (5V VIN), kein Deep-Sleep,
WiFi + Bluetooth, Dual-Core, 520 KB SRAM, 12-bit ADC.

**Standort:** Außeneinsatz, Campingplatz an der Weser, Jurtenzelt.

**Messintervall:** 60 Sekunden. Datenweg: ESP32 → HTTPS POST → n8n → Google Sheets.

### I²C Bus 0 (GPIO 21/22)
| Sensor | Adresse | Messgröße |
|--------|---------|-----------|
| AHT20 | 0x38 | Lufttemperatur, Luftfeuchte |
| BMP280 | 0x76 | Luftdruck |
| TCS34725 | 0x29 | RGB-Farbe, Helligkeit |
| TMP102 | 0x48 (ADD0→GND) | Bodentemperatur (gekabelt, nicht wasserdicht → vergießen) |

### I²C Bus 1 (GPIO 18/19) — separater Bus wegen Adresskonflikt
| Sensor | Adresse | Messgröße |
|--------|---------|-----------|
| TSL2591 | 0x29 | Lux (HDR) |

### ADC1 (alle mit WLAN nutzbar, Input-Only-Pins)
| GPIO | Sensor | Messgröße | Hinweis |
|------|--------|-----------|---------|
| 36 | HW-390 AOUT | Bodenfeuchte (kapazitiv) | spannungsabhängig, kalibrieren |
| 39 | HW-038 S | Wasserstand/Nässe (Leitfähigkeit) | korrodiert bei Dauerstrom → gaten |
| 34 | MAX9814 OUT | Schall (RMS) | AGC on-board |
| 35 | MiCS-5524 AO | Gas CO/VOC (KEIN CO₂) | über 10k/10k-Teiler, 60s Warmup |
| 32 | GUVA-S12SD OUT | UV | 0–1V → unteres ADC-Drittel |
| 33 | Raindrop AO | Regen-Intensität (analog) | |

### Digital
| GPIO | Richtung | Funktion |
|------|----------|----------|
| 27 | INPUT | Raindrop DO (Regen ja/nein, LM393 Open-Collector) |
| 26 | OUTPUT | MiCS-5524 EN (Heizer-Gate, dauerhaft HIGH) |
| 14 | OUTPUT | Power-Gate MOSFET HW-038+Raindrop (geplant, im Code nicht implementiert) |

**Nicht verbunden / frei:** IO13 (Servo SG90 war geplant, wurde entfernt), IO1/IO3 (USB-Seriell),
Strapping-Pins IO0/IO2/IO5/IO12/IO15.

### Stromversorgung
- 5V Netzteil → VIN des NodeMCU (Board-Regler liefert 3V3 für Logik/Sensoren)
- MiCS-5524-Heizer ebenfalls an 5V
- Aktuelle Stromaufnahme nicht gemessen
- Keine Akku- oder Solarfunktion vorgesehen
- Servo-Stromkreis (separate 5V + Stützkondensator) war geplant, entfällt

### Mechanische Randbedingungen
- Dauerhafter Außeneinsatz, feuchte Umgebung nahe Weser
- Winterfrost möglich (Temperatur < 0°C)
- Sommerliche direkte Sonneneinstrahlung möglich
- Gehäuse und Montage noch nicht festgelegt
- Thermische Entkopplung zwischen ESP32 und AHT20/BMP280 noch nicht gelöst
  (ESP32-D heizt stärker als C3 → Eigenerwärmung verfälscht Lufttemperatur/Feuchte)

### Sensorpositionen (offen)
- Positionierung aller Sensoren noch nicht festgelegt
- AHT20/BMP280 müssen in belüftetem, schattigem Gehäuse (Stevenson-Screen) sitzen
- TMP102 für Bodeneinsatz: nicht wasserdicht → muss vergossen werden
- HW-390 Bodenfeuchtesensor: Tiefe und Bodenkontakt noch nicht definiert
- Keine thermische Entkopplung zwischen ESP32-Wärmequelle und Luftsensoren bisher geplant

### ADC-Strategie
- Alle Analogsensoren über interne ESP32-ADC1-Kanäle (kein externer ADC)
- ESP32-ADC: bekannte Nichtlinearität (besonders <150mV und >2950mV), Rauschen, Referenzspannung streut
- ADC-Kalibrierung (esp_adc_cal) und Kennlinienkorrektur noch nicht implementiert
- analogReadMilliVolts() statt analogRead() für bessere Genauigkeit noch nicht geprüft
- GUVA-S12SD gibt 0–1V aus → liegt im untersten ADC-Drittel (schlechtester Linearitätsbereich)

### Zeitquelle
- Zeitbasis: NTP über WLAN (configTime, pool.ntp.org)
- Kein externer RTC-Baustein (kein DS3231 o.ä.) vorhanden
- Verhalten bei NTP-Ausfall: noch nicht definiert (aktuell Timestamp 1970 bei fehlendem Sync)
- Nach Neustart ohne WLAN: keine valide Zeitquelle

---

## Firmware-Istzustand (Arduino/ESP32, sketch umweltmonitor_basis.ino)

### Was implementiert ist
- Beide I²C-Busse korrekt initialisiert (Bus0: 21/22, Bus1: 18/19)
- ADC 12-bit, 11dB Attenuation (~0–3,1V)
- Alle Analogsensoren per avgRead() (N=16 Mittelwert)
- Schall: Single-Pass RMS mit double-Arithmetik (N=1500 Samples)
- TMP102 direkt per I²C (kein Lib), 12-bit, 0,0625°C/LSB
- HMAC-SHA256 über gesamten JSON-Body (inkl. seq + timestamp) via mbedTLS
- HTTPS mit setCACert() gegen hinterlegte Root-CA — kein setInsecure()
  (Root-CA vs. Intermediate-CA vs. Public-Key-Pinning noch nicht entschieden)
- seq-Counter: nur bei HTTP 2xx hochzählen + NVS-Persist (Replay-Schutz)
- Watchdog 30s (esp_task_wdt), millis()-basierter 60s-Takt
- Secrets (SSID, Pass, n8n-URL, HMAC-Key, CA-Cert) aus NVS per Preferences

### Bekannte Fehler/Lücken (vor erstem Hardware-Test beheben)

1. **LE Root CA = Platzhalter** — `"... TODO LE Root X1 ..."` im Code. HTTPS schlägt bei
   jedem POST fehl bis das echte ISRG Root X1 Zertifikat eingetragen ist.

2. **PIN_RAIN_D mit INPUT statt INPUT_PULLUP** — LM393 Open-Collector floated ohne Pull-up
   → zufällige Phantom-Regen-Events. Eine Zeile Fix.

3. **Power-Gate IO14 nicht implementiert** — HW-038 und Raindrop dauerhaft bestromt.
   Korrosionsrisiko bei Dauerbetrieb. pinMode(14, OUTPUT) + Gating fehlt komplett im Code.

4. **NTP-Sync nicht abgewartet** — configTime() kehrt sofort zurück. Erste Messung(en)
   bekommen Timestamp 1970-01-01T00:00:00Z wenn NTP noch nicht synchronisiert.

5. **Kein Guard für leere NVS-Secrets** — bei Erstinbetriebnahme ist hmacKey="". System
   signiert stillschweigend mit leerem Schlüssel, keine Fehlermeldung.

6. **JSON-Buffer 640 Bytes** — snprintf-Rückgabewert wird nicht geprüft. Bei Truncation
   geht ungültiges JSON raus.

7. **TSL2591: getLuminosity() gibt ADC-Counts, nicht Lux** — für echte Lux-Werte
   calculateLux(broadband, ir) verwenden.

8. **MICS_WARMUP_MS == SEND_INTERVAL_MS == 60s** — erste gesendete Messung hat immer
   gas_raw=null. Warmup reicht im Kaltstart/Außeneinsatz nicht (MiCS braucht 3–5 min).

### JSON-Payload (Struktur)
```json
{
  "device_id": "umweltmonitor_basis_01",
  "schema_version": 1,
  "timestamp": "2026-06-25T10:00:00Z",
  "seq": 42,
  "soil_moisture_raw": 1850,
  "soil_temp_c": 18.5,
  "water_level_raw": 200,
  "rain": false,
  "rain_intensity_raw": 3900,
  "air_temp_c": 22.3,
  "humidity_pct": 65.0,
  "pressure_hpa": 1013.2,
  "uv_raw": 420,
  "lux": 18500.0,
  "color_rgb": [120, 98, 74],
  "noise_rel": 31.4,
  "gas_raw": 1240
}
```

---

## Geplante Auswertungs-Roadmap (zu bewerten)

### Abgeleitete Metriken (alle serverseitig in n8n/Sheets/Python)

- **Taupunkt** aus AHT20 Temp + Feuchte (Magnus-Formel)
- **Luftdrucktrend** ΔP über 1h/3h/6h/24h (wichtiger als absoluter Druck)
- **Heat Index** aus Temp + Feuchte (Rothfusz-Algorithmus)
- **Jurten-Kondensationsindex** = Innenwandtemperatur − Taupunkt
  (Schwellen: >5°C=sicher, 2–5°C=beobachten, <2°C=Risiko)
- **Lüftungseffizienz** = Δ Feuchte + Δ Gas + Δ Temp nach Lüften
- **Trocknungsrate Boden** nach Regen (% Feuchtigkeitsverlust/Tag)
- **Regeneffizienz** = gemessener Regen vs. Anstieg Bodenfeuchte
  (zeigt Versickerung/Verdunstung/Abfluss)
- **Schall-Tagesprofil** — relative Klassen ruhig/normal/laut pro Stunde
- **Mikroklimaindex** = eigene Messung vs. nächste offizielle Wetterstation

### Hochwasser-Frühwarnung
Indirekt: Risikoscore aus Regenmenge↑ + Bodenfeuchte↑ + Regentage.
Direkt: Offizielle Pegeldaten der Weser automatisch abrufen (REST-API).

### TinyML auf dem ESP32 (Phase 4 der Roadmap)
Geplant: Anomalieerkennung (Autoencoder auf Temp/Feuchte/Druck/Boden),
später Mikro-Wettervorhersage (Drucktrend + Feuchte → Regenwahrscheinlichkeit).

### Datenarchitektur
1. ESP32 → messen, filtern, senden
2. n8n + Google Sheets → Speicherung, Alarme
3. Home Assistant → Dashboard, Automationen
4. Lenovo P53 Python (Pandas/Jupyter) → Langzeitanalyse

### Outdoor-Hardware
- Gehäuse: PETG statt PLA
- Stevenson-Screen (weiß, belüftet, schattiert) für Temperatursensoren
- Kippschaufel-Regenmesser für quantitative Regenmenge (mm)

---

## Deine Aufgabe

Bewerte technisch und ehrlich:

1. Sind die 8 Firmware-Fehler korrekt priorisiert? Fehlt etwas Kritisches?

2. Ist der **Jurten-Kondensationsindex** mit diesem Hardware-Set berechenbar?
   (Hinweis: TMP102 misst Boden, kein Wandsensor vorhanden)

3. Ist **Regeneffizienz** (Regen vs. Bodenfeuchte-Anstieg) mit Raindrop-DO + HW-390
   realistisch quantifizierbar?

4. Ist **TinyML auf dem ESP32** bei 60s-Intervall + n8n-Pipeline sinnvoll — oder ist
   serverseitige Python-Auswertung die bessere Wahl?

5. Skaliert **Google Sheets** für 1+ Jahr Langzeitdaten bei 60s-Intervall?
   (~525.600 Zeilen/Jahr)

6. Welche abgeleiteten Metriken sind mit den vorhandenen Sensoren **nicht** berechenbar,
   obwohl sie in der Roadmap stehen?

7. Was fehlt in der Roadmap, das du für diesen Standort (Fluss, Jurte, Außeneinsatz)
   für wichtig hältst?

8. **Welche drei Risiken würdest du vor dem Hardwareaufbau zuerst eliminieren?**

9. **Welche drei Messwerte des gesamten Systems liefern voraussichtlich den höchsten
   praktischen Nutzen** — bezogen auf den konkreten Alltag in der Jurte am Weserufer?

10. **Welche Teile wirken überkompliziert oder erzeugen mehr Aufwand als Erkenntnis?**
    Was würdest du weglassen oder vereinfachen?
