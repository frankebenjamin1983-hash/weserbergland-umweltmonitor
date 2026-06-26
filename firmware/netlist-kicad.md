# Netzliste — Umweltmonitor-Basismodul (für KiCad)

30-Pin NodeMCU-ESP32 (ESP32-D0WD). Alle Sensoren = Breakout-Module → **Pull-ups intern,
keine externen Widerstände** (Ausnahme: MiCS-AO-Teiler, s. u.). In KiCad: Power-Symbole
(+3V3/+5V/GND) statt langer Leitungen, globale Netz-Labels für Signale.

## Bauteile (Symbole)
| Ref | Bauteil | Pins (Modul) | Bus/Adresse |
|---|---|---|---|
| U1 | ESP32-D0WD NodeMCU (30 Pin) | siehe Pins unten | — |
| M1 | HW-390 Bodenfeuchte | AOUT, VCC, GND | analog |
| M2 | HW-038 Wasserstand | S, VCC, GND | analog |
| M3 | MAX9814 Schall | OUT, VDD, GND, GAIN, A/R | analog |
| M4 | MiCS-5524 Gas | AO, EN, VCC, GND | analog (5V) |
| M5 | GUVA-S12SD UV | OUT, VCC, GND | analog |
| M6 | Raindrop Regen | DO, AO, VCC, GND | digital + analog |
| M7 | **AHT20+BMP280 Combo** | VIN, GND, SDA, SCL | I²C-0 · 0x38 + 0x76 |
| M8 | TCS34725 Farbe | VIN, GND, SDA, SCL, LED, INT | I²C-0 · 0x29 |
| M9 | **TMP102 Bodentemp** (gekabelt) | VIN, GND, SDA, SCL, **ADD0**, **ALT** | I²C-0 · 0x48 |
| M10 | TSL2591 Lux | VIN, GND, SDA, SCL, INT, 3Vo | I²C-1 · 0x29 |

## Netze
| Netz | angeschlossene Pins |
|---|---|
| **+5V** | U1.VIN · M4.VCC (MiCS-Heizer) |
| **+3V3** | U1.3V3 · M1.VCC · M2.VCC · M3.VDD · M5.VCC · M6.VCC · M7.VIN · M8.VIN · M9.VIN · M10.VIN |
| **GND** | U1.GND (beide) · M1.GND · M2.GND · M3.GND · **M3.GAIN** · M4.GND · M5.GND · M6.GND · M7.GND · M8.GND · **M8.LED** · M9.GND · **M9.ADD0 (→Adresse 0x48)** · M10.GND |
| **I2C0_SDA** | U1.IO21 · M7.SDA · M8.SDA · M9.SDA |
| **I2C0_SCL** | U1.IO22 · M7.SCL · M8.SCL · M9.SCL |
| **I2C1_SDA** | U1.IO18 · M10.SDA |
| **I2C1_SCL** | U1.IO19 · M10.SCL |
| **SOIL_MOIST** | U1.IO36 · M1.AOUT |
| **WATER_LVL** | U1.IO39 · M2.S |
| **NOISE** | U1.IO34 · M3.OUT |
| **GAS** | U1.IO35 · M4.AO  *(über Spannungsteiler 10k/10k → max 2,5 V)* |
| **UV** | U1.IO32 · M5.OUT |
| **RAIN** | U1.IO27 · M6.DO |
| **RAIN_INTENSITY** | U1.IO33 · M6.AO  *(analoge Regenstärke, ADC1)* |
| **GAS_EN** | U1.IO26 · M4.EN  *(Polarität am Modul prüfen)* |

*Bodentemperatur (TMP102) läuft über **I2C0** — kein eigener Signal-Pin.*

## Nicht verbunden (n.c.)
- M3.A/R (offen = 1:4000) · M8.INT · **M9.ALT** · M10.INT · M10.3Vo
- U1: EN, TX0(IO1), RX0(IO3), **IO4**, IO23, IO25, IO14, IO12, IO13, IO5, IO17, IO16, IO2, IO15

## Hinweise
- **MiCS-AO-Teiler:** AO an 5V kann >3,3 V → 2 Widerstände (10k/10k = halbiert) vor IO35 (Pin-Schutz).
- **Keine sonstigen externen Pull-ups** (auf den Breakout-Boards vorhanden).
- **MAX9814 GAIN→GND** = 50 dB; **TCS34725 LED→GND** = Bord-LED aus.
- I²C-Sensoren an **3,3 V** (Bus = 3,3-V-Logik); nur MiCS-Heizer an 5V.
- **TMP102 nicht wasserdicht** → für Bodeneinsatz vergießen/abdichten.
- DS18B20 ist **nicht** Teil des Aufbaus (war nur ein Vorschlag).
