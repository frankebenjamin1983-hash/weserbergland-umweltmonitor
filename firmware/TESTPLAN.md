# TESTPLAN — UmweltMonitor Firmware (ESP32-D / WROOM-32)

Phase-0-Testplan: Zielsignale stehen **vor** dem Code. Fertig = **alle Einträge abgehakt + Doku
komplett**, nicht „fühlt sich gut an". Erledigte Einträge mit **Datum + Beleg** (Log/Lauf) abhaken,
nicht löschen. Belegungsplan/Entwurf: [`belegungsplan-und-entwurf.md`](belegungsplan-und-entwurf.md).

## Firmware-Grundfunktion (Datum: offen)
**Spezifikation:** ESP32-D liest alle Sensoren plausibel ein, steuert den Servo, gibt strukturiertes
JSON aus und sendet es in den Stations-Datenfluss (n8n). Mess-Semantik noch UNVERIFIZIERT.
**Output-Garantien:** JSON parsebar; feste Feldnamen; unkalibrierte Größen als `*_raw/_rel`;
ISO-Zeitstempel; **KEIN Feld „co2"** (MiCS-5524 = CO/brennbare Gase); alle Analogeingänge auf ADC1
(ADC2 bei WLAN gesperrt); Werte im 0–3,1-V-Fenster.

### Sensorik — Reaktionstests (live)
| # | Test | Art | Erfolgssignal | Status |
|---|------|-----|---------------|--------|
| 1 | HW-390 Boden trocken → nass | live | `soil_moisture_raw` ändert sich deutlich in erwartete Richtung | [ ] |
| 2 | HW-038 Wasser trocken → eingetaucht | live | `water_level_raw` steigt deutlich | [ ] |
| 3 | Raindrop trocken → Tropfen auf Platte | live | `rain_now` wechselt false → true (DO) | [ ] |
| 4 | MAX9814 Stille → lautes Geräusch | live | `sound_level_rel` (RMS) steigt deutlich | [ ] |
| 5 | MiCS-5524 nach Warmup, Baseline → nahe Gasquelle (z. B. Feuerzeuggas/Alkohol) | live | `gas_raw` ändert sich deutlich | [ ] |
| 6 | GUVA-S12SD Schatten → Sonne/UV-Quelle | live | `uv_raw` steigt deutlich | [ ] |
| 7 | TCS34725 rote vs. blaue Fläche; hell/dunkel | live | `color_rgb` plausibel; `light_clear` reagiert auf Helligkeit | [ ] |
| 8 | ADC-Plausibilität bei aktivem WLAN | live | alle Kanäle stabil, kein Übersprechen, nur ADC1 | [ ] |

### Aktorik & Regen (live)
| # | Test | Art | Erfolgssignal | Status |
|---|------|-----|---------------|--------|
| 9 | Füllstand ≥ High-Schwelle | live | Servo kippt + kehrt zurück; `rain_dumps_count`++ | [ ] |
| 10 | Hysterese | live | kein erneuter Dump bis Füllstand ≤ Low-Schwelle | [ ] |
| 11 | Frost-Sperre (Temp < 0 °C, Quelle gem. offener Entscheidung) | live | **KEIN** Dump bei Frost | [ ] |
| 12 | mm-Berechnung nach Kalibrierung | live | `rain_mm` = dumps×Dump-Vol/Fangfläche stimmt mit bekannter Wassermenge | [ ] |
| 13 | Korrosions-Gating | auto/live | Gate-Pin nur im Messfenster HIGH, HW-038/Raindrop sonst stromlos | [ ] |

### Datenweg (live/auto)
| # | Test | Art | Erfolgssignal | Status |
|---|------|-----|---------------|--------|
| 14 | JSON wohlgeformt + ehrliche Felder | auto | parsebar; **kein** `co2`; unkalibrierte als `*_raw/_rel` | [ ] |
| 15 | POST an n8n | live | Eintrag erscheint im Ziel (Sheet/Webhook), Werte plausibel | [ ] |
| 16 | Zeitstempel | live | ISO-Zeit korrekt (Gerät-NTP oder n8n) | [ ] |

### Robustheit / Failure (live) — die wertvolleren Tests
| # | Test | Art | Erfolgssignal | Status |
|---|------|-----|---------------|--------|
| 17 | WLAN-Ausfall mitten im Betrieb | live | Retry, kein Crash/Hang; nächster Zyklus sendet wieder | [ ] |
| 18 | Servo-Brownout | live | Servo-Bewegung löst **keinen** ESP32-Reset aus (separate 5 V + Stützkondensator) | [ ] |
| 19 | n8n nicht erreichbar | live | POST scheitert sauber, kein Crash, Log-Eintrag (ggf. Puffer) | [ ] |
| 20 | Sensor abgesteckt | live | Kanal liefert Rail-Wert → als unplausibel erkennbar/markiert | [ ] |
| 21 | MiCS ohne Warmup gemessen | auto/live | Wert verworfen/markiert | [ ] |

## Abhängigkeiten zu offenen Entscheidungen
- Test 5/6 Auflösung hängt an **ADS1115-Variante A/B**.
- Test 11 (Frost) hängt an **Temp-Quelle** (Stations-Webhook / DS18B20 / entfällt in v1).
- Test 4/17 hängen an **Mic-Modus** (Deep-Sleep vs. kontinuierlich) und **BT-Rolle**.
Siehe „Offene Entscheidungen" im Belegungsplan-Dokument.

## Konsolidierung & neue Sensoren (Datum: offen)
| # | Test | Art | Erfolgssignal | Status |
|---|------|-----|---------------|--------|
| 22 | TSL2591 hell → dunkel | live | `lux`-Wert reagiert plausibel über großen Bereich (HDR) | [ ] |
| 23 | Zwei I²C-Busse / kein Adresskonflikt | auto | TSL2591 auf Bus 1 (GPIO16/17) **und** TCS34725 auf Bus 0 per Scan getrennt erreichbar | [ ] |
| 24 | Stations-Sensoren übernommen | live | AHT20 (Temp/Feuchte), BMP280 (Druck) liefern plausible Werte; `diff_temperature`-Logik wie bisher | [ ] |
| 25 | Datenvertrag zusammengeführt | auto | JSON enthält Stations- **und** Umweltfelder, keine Namenskollision | [ ] |
| 26 | Eigenerwärmung (Konsolidierung) | live | AHT20-Temp vs. externes Referenz-Thermometer: Bias durch ESP32-D-Wärme quantifiziert/akzeptabel | [ ] |
| 27 | Frost-Sperre via AHT20 | live | bei AHT20-Temp < 0 °C **kein** Servo-Dump | [ ] |

## Funk-Segment / Sternnetz — ⚠️ SUPERSEDED (kein Funk mehr)
*Einzel-Basismodul, Bodensonde gekabelt → Tests 28–33 gegenstandslos, bleiben als Audit-Trail. Maßgeblich: prompts/firmware-prompt.md. Der Schreibpfad-Schutz (HTTPS/HMAC/Replay/NVS) bleibt gültig und wird beim Basismodul-POST getestet.*
| # | Test | Art | Erfolgssignal | Status |
|---|------|-----|---------------|--------|
| 28 | Gefälschter Frame (falsche Signatur) | live | Gateway verwirft + loggt, kein Wert übernommen | [ ] |
| 29 | Replay (gültiger Frame erneut gesendet) | live | abgelehnt (Zähler ≤ letzter Stand) | [ ] |
| 30 | Unbekannter Schlüssel / ungepeerte Sonde | live | kein Peering, Frame ignoriert | [ ] |
| 31 | Reichweite je Sonde am echten Standort | live | RSSI/Verlustrate dokumentiert (Boden tief / Himmel hoch) | [ ] |
| 32 | Kompartmentierung | auto | Sondenknoten enthält **keine** n8n-Credentials (Code/NVS-Review) | [ ] |
| 33 | WLAN+ESP-NOW-Koexistenz am Gateway | live | Uplink + Funk laufen stabil auf **demselben Kanal**, keine Frame-Verluste | [ ] |

> Funkprotokoll = **ESP-NOW** (AES PMK/LMK + monotoner Zähler gegen Replay). Tests 28–30 gelten dafür.
