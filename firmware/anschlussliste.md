# Anschlussliste — Umweltmonitor-Basismodul (30-Pin NodeMCU-ESP32)

Maßgebliche Verdrahtungsquelle. Board: ESP32-D0WD NodeMCU, 30 Pin. Alle Sensoren = Breakout-Module
→ Pull-ups intern (keine externen), außer MiCS-AO-Teiler.

## Versorgung
- **VIN ← 5 V Netzteil** (Board-Regler **und** MiCS-Heizer).
- **3V3 (Board-Pin)** → VCC/VIN aller Sensoren außer MiCS-Heizer.
- **GND** → gemeinsame Masse.

## I²C-Busse
- **Bus 0 = IO21 (SDA) / IO22 (SCL):** AHT20+BMP280-Combo (0x38 + 0x76) · TCS34725 (0x29) · TMP102 (0x48)
- **Bus 1 = IO18 (SDA) / IO19 (SCL):** TSL2591 (0x29)

## Module (alle Pins)
| Modul | Pin | → Ziel |
|---|---|---|
| **HW-390** Bodenfeuchte | AOUT / VCC / GND | **IO36** / 3V3 / GND |
| **HW-038** Wasserstand | S / VCC / GND | **IO39** / 3V3 / GND |
| **MAX9814** Schall | OUT / GAIN / A/R / VDD / GND | **IO34** / **GND (50 dB)** / offen / 3V3 / GND |
| **MiCS-5524** Gas (CO) | AO / EN / VCC / GND | **IO35** (über Teiler 10k/10k) / **IO26** / **5V** / GND |
| **GUVA-S12SD** UV | OUT / VCC / GND | **IO32** / 3V3 / GND |
| **Raindrop** Regen | DO / AO / VCC / GND | **IO27** / **IO33** (Intensität) / 3V3 / GND |
| **AHT20+BMP280 Combo** (Bus 0) | SDA / SCL / VIN / GND | **IO21** / **IO22** / **3V3** / GND |
| **TCS34725** RGB (Bus 0) | SDA / SCL / LED / INT / VIN / GND | **IO21** / **IO22** / **GND (LED aus)** / offen / **3V3** / GND |
| **TMP102** Bodentemp (Bus 0, gekabelt) | SDA / SCL / VIN / GND / ADD0 / ALT | **IO21** / **IO22** / **3V3** / GND / **GND (→0x48)** / offen |
| **TSL2591** Lux (Bus 1) | SDA / SCL / INT / VIN / GND | **IO18** / **IO19** / offen / **3V3** / GND |

## Frei / nicht belegen
- Frei nutzbar: **IO4**, IO23, IO25, IO14, IO13, IO16, IO17.
- **TX0/RX0 (IO1/IO3):** USB-Seriell — nicht belegen.
- Strapping (Boot-Pegel beachten): IO12, IO5, IO2, IO15.

## Hinweise
- **MiCS-AO:** AO an 5V kann >3,3 V → Spannungsteiler **10k/10k** (max 2,5 V) vor IO35.
- **EN-Polarität** am MiCS prüfen (HIGH = an angenommen).
- **TMP102 nicht wasserdicht** → für Boden vergießen/abdichten.
- **DS18B20** ist nicht Teil des Aufbaus (war nur ein Vorschlag).
