# Stückliste (BOM) — Umweltmonitor-Basismodul

Board: ESP32-D0WD NodeMCU (30 Pin). Versorgung: 5-V-Netzteil an VIN. Alle Sensoren = Breakout-Module
(Pull-ups intern). Stand 2026-06-25.

| Pos | Bauteil | Menge | Funktion | Anschluss (ESP) | Bus/Adresse | Versorgung | Hinweis |
|---|---|---|---|---|---|---|---|
| 1 | **ESP32-D0WD NodeMCU** (30 Pin) | 1 | MCU + WLAN-Uplink | — | — | 5 V (VIN) | Basismodul; TX0/RX0 = USB-Seriell, frei lassen |
| 2 | **HW-390** kapazitiv v2.0 | 1 | Bodenfeuchte | IO36 (AOUT) | analog ADC1 | 3V3 | Wert spannungsabhängig → kalibrieren |
| 3 | **HW-038** | 1 | Wasserstand/Nässe | IO39 (S) | analog ADC1 | 3V3 | Leitfähigkeitstyp; korrodiert (nicht dauerhaft fluten) |
| 4 | **Raindrop-Modul** (LM393) | 1 | Regen + Intensität | IO27 (DO) + IO33 (AO) | digital + analog | 3V3 | DO = ja/nein, AO = Stärke; **kein** mm |
| 5 | **MAX9814** | 1 | Schallpegel | IO34 (OUT) | analog ADC1 | 3V3 | GAIN→GND (50 dB), A/R offen; nur relativ |
| 6 | **MiCS-5524** | 1 | Gas (CO/brennbar) | IO35 (AO) + IO26 (EN) | analog ADC1 | **5 V** | Heizer; AO über **Teiler** (Pos 12); nur indikativ |
| 7 | **GUVA-S12SD** | 1 | UV | IO32 (OUT) | analog ADC1 | 3V3 | 0–1 V; UV-Index n. Kennlinie |
| 8 | **AHT20+BMP280 Combo** | 1 | Temp/Feuchte/Druck | IO21/IO22 | I²C-0 · 0x38 + **0x77** | 3V3 | ein Board, zwei Adressen; BMP280 **verifiziert 0x77** (nicht 0x76) |
| 9 | **TCS34725** | 1 | Farbe/Helligkeit | IO21/IO22 | I²C-0 · 0x29 | 3V3 | LED→GND (Bord-LED aus), INT offen |
| 10 | **TMP102** | 1 | Bodentemperatur | IO21/IO22 | I²C-0 · 0x48 | 3V3 | ADD0→GND (=0x48), ALT offen; **abdichten** fürs Erdreich |
| 11 | **TSL2591** | 1 | Lux (HDR) | IO18/IO19 | I²C-1 · 0x29 | 3V3 | eigener Bus (0x29-Konflikt mit TCS) |
| 12 | **MB85RC256V FRAM** | 1 | Persistenz (seq-Zähler, Ringpuffer) | IO21/IO22 | I²C-0 · **0x50** | 3V3 | 32 KB; 10¹³ Schreibzyklen; A0/A1/A2 → GND |
| 13 | **ST7789 1.47" TFT** | 1 | Display (Temp/Feuchte/Lux/WLAN) | GPIO13/14/25/16/4/17 | SPI-HSPI | 3V3 | 172×320 px; CS=25 DC=16 (Strapping-Pins GPIO15/2 gemieden) |
| 14 | **Widerstand 10 kΩ** | 2 | MiCS-AO-Teiler (5V→2,5V) | an IO35 | — | — | Pin-Schutz; nur falls AO >3,3 V |
| 14 | **5-V-Netzteil** | 1 | Versorgung | VIN/GND | — | — | muss MiCS-Heizerstrom mitliefern (~30–80 mA) |
| 15 | Kabel/Stecker, Gehäuse, Bodensonden-Kabel | n. B. | Aufbau | — | — | — | TMP102 + Bodenfeuchte ins Erdreich (gekabelt) |

## I²C-Übersicht
- **Bus 0 (IO21/IO22):** AHT20 0x38 · BMP280 **0x77** · TCS34725 0x29 · TMP102 0x48 · **MB85RC256V FRAM 0x50**
- **Bus 1 (IO18/IO19):** TSL2591 0x29

## Was NICHT verbaut ist (zur Klarstellung)
- **DS18B20** — war nur ein Vorschlag, nicht Teil des Aufbaus (Bodentemp = TMP102).
- **SGP40 / CO₂ / eCO₂** — kein solcher Sensor vorhanden; Gas = nur MiCS-5524.
- **Servo / Regen-Entleerung** — vorerst weggelassen.
- **Externe I²C-Pull-ups** — nicht nötig (auf den Modulen).

## Offene Realität-Checks (vor dem Löten verifizieren)
- MiCS-AO-Spannung messen → Teiler nur, wenn >3,3 V.
- MiCS-EN-Polarität am Modul (HIGH = an angenommen).
- Raindrop-DO-Logik (LOW = nass angenommen).
- TMP102 für Bodeneinsatz wasserdicht vergießen.
