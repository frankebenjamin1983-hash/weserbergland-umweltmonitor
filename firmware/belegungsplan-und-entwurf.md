# UmweltMonitor — Belegungsplan & Firmware-Entwurf

> ## ✅ Maßgeblicher Stand (2026-06-25) — das gilt
> - Board: **ESP32-D0WD NodeMCU, 30-Pin**, Netzbetrieb.
> - **Kein Servo, kein MOSFET/Power-Gate, kein ADS1115, kein DS18B20, kein SGP40.** → **IO13 + IO14 frei.**
> - **Keine externen Widerstände** außer optionalem MiCS-AO-Teiler (10k/10k); I²C-Pull-ups intern (Breakout-Module).
> - **HW-038 + Raindrop: VCC direkt an 3V3** (kein Gating; Korrosion über Platzierung/Abdichtung mindern).
> - **MiCS-Heizer: EN (IO26) dauerhaft HIGH** (Netzbetrieb, gewollt — keine Bug).
> - **Bodentemperatur: TMP102** (I²C 0x48), nicht DS18B20.
> - **Maßgeblich für die Verdrahtung:** `anschlussliste.md` + `netlist-kicad.md` + `umweltmonitor-basismodul-schema.svg`.
> - Alle Tabellen/Notizen weiter unten (C3, ADS1115, Servo, Sternnetz, Power-Gate) sind **historisch** und nur Audit-Trail.

> 🔒 **Final-Stand (2026-06-24): Einzel-Basismodul, kein Funk.** Ein ESP32-Basismodul,
> Bodensonde **gekabelt**, Himmelssonde integriert. Maßgeblich: [`../prompts/firmware-prompt.md`](../prompts/firmware-prompt.md).
> Deltas ggü. dem Schema unten: **Servo/Entleerung raus** (GPIO13 frei) → Regen = Raindrop-DO +
> HW-038-Füllstand **ohne** Auto-Dump (kein mm); **Intervall ~1 min**, Versorgung **Netz**.
> Servo-/Sternnetz-Zeilen unten gegenstandslos. *(SGP40 wieder entfernt — nicht vorhanden.)*
> **Board korrigiert:** Es ist ein **30-Pin-NodeMCU-ESP32** (ESP32-D0WD), **nicht** das 38-Pin-DevKitC.
> Maßgebliches Pinout: [`umweltmonitor-basismodul-schema.svg`](umweltmonitor-basismodul-schema.svg).
> 30-Pin heißt: **keine Flash-Pins** (IO6–11) herausgeführt, 3V3 unten rechts, VIN unten links, TX0/RX0 = USB-Seriell (frei lassen).

> **MCU: ESP32-D (WROOM-32, WiFi + Bluetooth)** — vom Nutzer gewählt (2026-06-24), bewusste
> Abweichung von der C3-Stations-Basis. Folgen: mehr Strom/Wärme als C3; WiFi+BT gleichzeitig
> belastet Funk/RAM (für die Datenübertragung reicht WLAN → BT nur für lokale Konfig/Readout);
> **8 ADC1-Kanäle trotz WLAN → ADS1115 nur noch optional**.
>
> *Stand: Entwurf. Mess-Semantik (Gas≠CO₂, Schall=relativ, mm/UV erst nach Kalibrierung) ist
> NOCH NICHT verifiziert — siehe README-Abschnitt „Noch zu verifizieren".*

## ⚠ Konsolidierung & neue I²C-Sensoren (2026-06-24)
- **Konsolidiert (Nutzer-Entscheidung):** Dieses **eine** ESP32-D-Board trägt **Stations-Sensoren
  + Umweltsensoren** und ersetzt den alten C3. → Belegungsplan/Pinout sind **noch UNVOLLSTÄNDIG**:
  die Sensoren der Wetterstation fehlen; deren Liste + aktuelle Pins müssen ergänzt werden. Der
  Datenvertrag führt Stations- und Umweltfelder zusammen.
- **TSL2591 hinzugefügt** (I²C, präziser Lux, hoher Dynamikbereich). **ACHTUNG Adresskonflikt:**
  TSL2591 **und** TCS34725 nutzen beide I²C-Adresse **0x29** und sind auf Standard-Breakouts nicht
  umadressierbar → sie können **nicht** einfach am selben Bus liegen. Optionen:
  (a) **zweiter I²C-Bus** (ESP32 hat `Wire` + `Wire1`) — z. B. TSL2591 an Bus 0, TCS34725 an Bus 1;
  (b) **TCA9548A** I²C-Mux; (c) **TCS34725 weglassen**, falls nur Helligkeit (Lux) statt Farbe
  gebraucht wird → TSL2591 übernimmt die Lux-Rolle, Konflikt entfällt.

## ESP32-Pin-Realität (WROOM-32) — Randbedingungen
- **Input-only, ideal für Analog:** GPIO34, 35, 36 (VP), 39 (VN) — kein Output, **kein interner Pull-up**.
- **ADC1 (mit WLAN nutzbar):** GPIO32, 33, 34, 35, 36, 39 (GPIO37/38 auf WROOM-32 i. d. R. nicht herausgeführt).
- **ADC2 = bei aktivem WLAN für Analog GESPERRT** → keine Analogsensoren dort.
- **Strapping/meiden:** GPIO0, 2, 5, 12 (MTDI/Flash-Spannung!), 15 — nicht für kritische Funktionen.

## Belegungsplan — Variante A (nativ, empfohlen) — ohne ADS1115

| Funktion | Sensor/Aktor | GPIO | Typ | Hinweis |
|---|---|---|---|---|
| Bodenfeuchte | HW-390 AOUT | **36 (VP)** | ADC1_CH0, input-only | `analogReadMilliVolts`, mehrfach mitteln; stabile 3,3 V |
| Wasserstand | HW-038 S | **39 (VN)** | ADC1_CH3, input-only | nur **gated** bestromen (Korrosion) |
| Gas (CO/brennbar/VOC) | MiCS-5524 AOUT | **35** | ADC1_CH7, input-only | **Heizer-Warmup vor Messung**; nur relativ |
| UV | GUVA-S12SD AOUT | **32** | ADC1_CH4 | 0–1 V → grobe Auflösung am 12-bit-ADC (Var. B besser) |
| Schall | MAX9814 OUT | **34** | ADC1_CH6, input-only | **schnelles Sampling + RMS**, kein Einzel-DC-Wert |
| Regen (Schwelle) | Raindrop **DO** | **27** | Digital-In, `INPUT_PULLUP` | DO statt AO spart Kanal (LM393 open-collector); gated |
| I²C SDA | TCS34725 + TSL2591* | **21** | I²C | *beide 0x29 → 2. Bus / Mux / eins weglassen |
| I²C SCL | TCS34725 + TSL2591* | **22** | I²C | + Stations-I²C-Sensoren (Liste offen) |
| Servo | SG90 | **13** | LEDC 50 Hz (PWM-Out) | **eigene 5 V + Stützkondensator**, gemeinsame Masse |
| Power-Gate | HW-038 + Raindrop | **14** | Digital-Out → MOSFET | HIGH nur zur Messung |
| Heizer-Gate (optional) | MiCS-5524 | **26** | Digital-Out → MOSFET | falls Heizer geschaltet werden soll |

Reserve: GPIO33 (ADC1_CH5) frei — z. B. für Raindrop-AO, falls Intensität analog gewünscht.

## Konsolidierter Belegungsplan (Stations- + Umweltsensoren, 2 I²C-Busse)
Stations-Sensoren (aus AntiWetter-README): **AHT20** (Temp/Feuchte, 0x38), **BMP280** (Druck, 0x76),
**LCD16x2-Backpack** (0x27, *optional — laut Einsatz „Display entfernt", klären*). Alles I²C.
*(DHT22/AM2302 aus der README ist im aktuellen Build NICHT enthalten.)*

**I²C-Bus 0 (`Wire`, GPIO21 SDA / GPIO22 SCL):** AHT20 (0x38) · BMP280 (0x76) · TCS34725 (0x29) · LCD (0x27)
**I²C-Bus 1 (`Wire1`, GPIO18 SDA / GPIO19 SCL):** TSL2591 (0x29) — wegen 0x29-Konflikt getrennt
*(verschoben von IO16/17, da IO17 am realen Board „HW-394" nicht ging; IO18/19 frei. Eigene Pull-ups 4k7 nötig.)*

| Funktion | GPIO | Typ | Quelle |
|---|---|---|---|
| AHT20 + BMP280 + TCS34725 + LCD | 21/22 | I²C-Bus 0 | Station + Umwelt |
| TSL2591 | 18/19 | I²C-Bus 1 | Umwelt |
| Raindrop DO | 27 | Digital-In | Umwelt |
| Power-Gate (HW-038 + Raindrop) | 14 | Digital-Out → MOSFET | Umwelt |
| MiCS Heizer-Gate (optional) | 26 | Digital-Out | Umwelt |
| Servo SG90 | 13 | PWM (LEDC) | Umwelt |
| Analog ADC1 (Boden/Wasser/Schall/Gas/UV) | 36/39/34/35/32 | ADC1 | Umwelt |

Strapping (0/2/5/12/15) frei. **Hinweis WROOM-32:** GPIO16/17 frei; bei einem **WROVER**-Modul sind
16/17 vom PSRAM belegt → dann anderes Bus-1-Paar wählen.

**Folgen der Konsolidierung:**
- **Frost-Sperre** nutzt jetzt die **AHT20-Temperatur** → offene Entscheidung #2 erledigt.
- **Eigenerwärmung wieder relevant:** Board misst Lufttemp/-feuchte (AHT20/BMP280) und der
  ESP32-D heizt stärker als der C3 → thermische Trennung/Strahlungsschutz beachten (Gehäuse-Chat).
- **Datenvertrag** führt Stations-Felder (temp/hum/press/`diff_temperature`) und Umweltfelder zusammen.

## Belegungsplan — Variante B (hybrid, höhere Präzision)
ADS1115 (I²C 0x48) für die kleinen/qualitätskritischen Signale, Rest nativ:
- **ADS1115 A0 = MiCS-5524, A1 = GUVA-S12SD** (16-bit + PGA → bessere Auflösung bei 0–1 V).
- Nativ: **MAX9814 → GPIO34** (muss nativ bleiben, schnelles Sampling), HW-390 → 36, HW-038 → 39.
- Frei dadurch: GPIO32, GPIO35.
- Nur sinnvoll, wenn UV/Gas feiner aufgelöst werden sollen — sonst Variante A.

## Stromversorgung (Topologie)
- ESP32 = 3,3 V Logik; ADC mit **11 dB Dämpfung** → ~0–3,1 V nutzbar (nahe den Rändern nichtlinear → `analogReadMilliVolts` + Mittelung).
- Sensoren an stabiler 3,3 V (HW-390 ist spannungsabhängig!); **Servo an separater 5-V-Schiene** + Elko (≥470 µF) gegen Brownout; **gemeinsame Masse**.
- MAX9814 so versorgen, dass der Ausgangs-Hub im 0–3,1-V-Fenster bleibt.

## Firmware-Entwurf (Struktur)
**Libraries:** `WiFi.h`, `HTTPClient.h`, `Wire.h`, `Adafruit_TCS34725`, `ESP32Servo` (oder LEDC direkt), optional `Adafruit_ADS1X15` (Var. B), optional NTP für ISO-Zeit.

**setup():**
1. `Serial`, `Wire.begin(21,22)`, `tcs.begin()`, Servo an GPIO13 (LEDC 50 Hz), `analogReadResolution(12)` + Attenuation 11 dB.
2. `pinMode` Gating (14, 26 = OUTPUT, LOW), Raindrop-DO (27 = `INPUT_PULLUP`).
3. WiFi verbinden; (NTP-Sync, falls Zeitstempel auf dem Gerät statt in n8n).

**Messzyklus (~10 min; `millis()` oder Deep-Sleep):**
1. Gating HIGH → HW-038/Raindrop bestromen; **MiCS-Heizer-Warmup** abwarten.
2. Langsame Analogkanäle lesen (je N-fach mitteln): Boden, Wasser, Gas, UV.
3. **Mic-Burst:** GPIO34 für ~100 ms mit einigen kHz sampeln → RMS/Peak → `sound_level_rel`.
4. TCS34725 (RGB + Clear) über I²C.
5. Raindrop-DO (Regen ja/nein); optional AO.
6. **Regen-/Servo-Logik:** HW-038-Füllstand lesen; bei `level ≥ high_thresh` **und nicht Frost**
   → Servo-Dump (Winkel → warten → zurück), `dumps++`; **Hysterese** (erst wieder ab `level ≤ low_thresh`).
   `rain_mm = dumps × dump_vol / fang_flaeche` (Kalibrierkonstanten).
7. Gating LOW (Sensoren stromlos).
8. JSON bauen → POST an n8n (oder Serial).
9. Schlafen bis zum nächsten Zyklus.

**Datenvertrag (Vorschlag — ehrliche Feldnamen, KEIN „co2"):**
```json
{
  "device": "umweltmonitor_erder",
  "timestamp": "2026-06-24T14:05:00+02:00",
  "soil_moisture_raw": 0,
  "water_level_raw": 0,
  "rain_now": false,
  "rain_mm_accumulated": 0.0,
  "rain_dumps_count": 0,
  "sound_level_rel": 0.0,
  "gas_raw": 0,
  "uv_raw": 0,
  "light_clear": 0,
  "color_rgb": [0, 0, 0]
}
```

## Offene Entscheidungen (vor dem Coden klären)
1. **ADS1115: Variante A oder B?** (A = einfacher/weniger Teile; B = bessere Auflösung GUVA/MiCS).
2. ~~Frost-Sperre-Temperaturquelle~~ **ERLEDIGT (Konsolidierung):** Frost-Sperre nutzt die AHT20-Temperatur des Boards.
3. **Bluetooth-Rolle:** Daten gehen per WLAN an n8n. BT nur für lokale Konfig/Readout? Dann BLE
   (leichter im WiFi-Coexistence) statt BT Classic erwägen.
4. **Mic:** kontinuierliche Lärmüberwachung (kein Deep-Sleep) vs. periodischer Schnappschuss (Deep-Sleep ok).
5. **Zeitstempel:** auf dem Gerät (NTP) oder von n8n vergeben?
6. **Kalibrierkonstanten** (Fangfläche, Dump-Volumen, Schwellen, UV-/Gas-Kennlinien) — erst am Aufbau messbar.
7. ~~I²C-Adresskonflikt TSL2591/TCS34725~~ **ERLEDIGT:** Entscheidung (a) — TSL2591 auf 2. I²C-Bus (`Wire1`, GPIO16/17).
8. ~~Stations-Sensorliste~~ **ERLEDIGT:** AHT20/BMP280/DHT22/LCD übernommen (siehe konsolidierter Belegungsplan). LCD-Behalt noch klären.

## Hinweis Eigenerwärmung
Anders als bei der Station misst der Umweltmonitor **keine Lufttemperatur/-feuchte** → Eigenerwärmung
ist hier unkritischer. **Aber:** MiCS-Gasmesswerte driften mit der Temperatur → bei der Auswertung
berücksichtigen (oder Temp vom Webhook beiziehen).
