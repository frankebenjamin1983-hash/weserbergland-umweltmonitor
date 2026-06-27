# Anschlussliste — Umweltmonitor-Basismodul

Board: ESP32-D0WD NodeMCU 30-Pin. Versorgung: 5 V an VIN. Stand: 2026-06-27.

## Versorgung

| Von | Nach | Hinweis |
|-----|------|---------|
| Netzteil 5 V | VIN + GND | trägt MiCS-Heizer mit (~30–80 mA) |
| Board 3V3 | VCC aller Sensoren außer MiCS | — |
| GND | gemeinsame Masse | — |

## I²C-Bus 0 — GPIO21 (SDA) / GPIO22 (SCL)

| Modul | Adresse | Pins am Modul |
|-------|---------|---------------|
| AHT20 + BMP280 Combo | 0x38 + **0x77** | SDA/SCL/VCC/GND |
| TCS34725 | 0x29 | SDA/SCL/VCC/GND (LED→GND) |
| TMP102 | 0x48 | SDA/SCL/VCC/GND (ADD0→GND) |
| MB85RC256V FRAM | 0x50 | SDA/SCL/VCC/GND (A0/A1/A2→GND, WP→GND) |

> BMP280 verifiziert auf **0x77** (nicht 0x76).

## I²C-Bus 1 — GPIO18 (SDA) / GPIO19 (SCL)

| Modul | Adresse | Grund |
|-------|---------|-------|
| TSL2591 | 0x29 | 0x29-Konflikt mit TCS34725 → eigener Bus |

## SPI-HSPI — ST7789 1.47" TFT

| Signal | GPIO | Hinweis |
|--------|------|---------|
| SCK | 14 | — |
| MOSI | 13 | — |
| CS | **25** | GPIO15 gemieden (Strapping-Pin) |
| DC | **16** | GPIO2 gemieden (Strapping-Pin + Onboard-LED) |
| RST | 4 | — |
| BL | 17 | Backlight; prüfen ob Transistor nötig (GPIO-Limit 40 mA) |
| VCC | 3V3 | — |
| GND | GND | — |

## Analog (ADC1)

| Modul | GPIO | Signal | Versorgung |
|-------|------|--------|------------|
| HW-390 Bodenfeuchte | 36 | AOUT | 3V3 |
| HW-038 Wasserstand | 39 | S | 3V3 |
| MAX9814 Schall | 34 | OUT (GAIN→GND = 50 dB) | 3V3 |
| MiCS-5524 Gas | 35 | AO (ggf. 10k/10k-Teiler wenn >3,3 V) | **5 V** |
| GUVA-S12SD UV | 32 | OUT | 3V3 |
| Raindrop AO | 33 | AO (Intensität) | 3V3 |

## Digital

| Modul | GPIO | Signal | Richtung |
|-------|------|--------|----------|
| Raindrop DO | 27 | DO (LOW = nass, LM393 open-collector) | IN PULLUP |
| MiCS-5524 EN | 26 | Enable (HIGH = Heizer an) | OUT |

## Reserviert

| GPIO | Funktion |
|------|----------|
| 1 (TX0) | USB-Serial — nicht belegen |
| 3 (RX0) | USB-Serial — nicht belegen |

## Hinweise

- ADC2-Pins bei aktivem WLAN gesperrt → nur ADC1 (GPIO32–39) verwenden.
- HW-038 und Raindrop korrodieren bei Dauerstrom → bei Gelegenheit auf Schaltung umstellen.
- TMP102 für Bodeneinsatz wasserdicht vergießen (Epoxid o.ä.).
- MiCS-5524: AO-Spannung zuerst messen, Teiler nur wenn >3,3 V.
