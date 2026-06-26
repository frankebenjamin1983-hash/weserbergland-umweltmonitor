# Leistungsbudget — Umweltmonitor (Stromversorgung-Check)

Ziel: Stromversorgung als Fehlerquelle aus-/einschätzen. **Werte sind Datenblatt-/Typ-Schätzungen, nicht gemessen.**
Bezug: Strom am **5-V-Eingang** (VIN). Der Onboard-LDO (AMS1117) wandelt 5 V → 3,3 V; bei einem Linearregler ist
der 5-V-Eingangsstrom ≈ 3,3-V-Strom + Ruhestrom.

| Komponente | Strom @5V (typ.) | Hinweis |
|---|---:|---|
| ESP32-D0WD, WLAN aktiv (Mittel) | ~150 mA | TX-Bursts kurzzeitig ~300 mA |
| Onboard-LDO Ruhe + Board-LED | ~8 mA | |
| **MiCS-5524 Heizer + Amp** | **~35 mA** | EN dauerhaft HIGH → **kontinuierlich** |
| AHT20 + BMP280 (Combo) | ~1 mA | |
| TCS34725 (Bord-LED aus) | ~1 mA | |
| TSL2591 | ~1 mA | |
| TMP102 | ~0,01 mA | vernachlässigbar |
| MAX9814 (Mikrofon-Verstärker) | ~4 mA | |
| HW-390 (kapazitiv) | ~5 mA | |
| HW-038 | ~5 mA | |
| GUVA-S12SD (UV) | ~1 mA | |
| Raindrop (LM393 + Power-LED) | ~8 mA | |
| diverse Modul-Power-LEDs | ~15 mA | mehrere Breakouts haben eine LED |
| **Summe Mittel** | **~234 mA ≈ 1,17 W** | |
| **Summe Peak (WLAN-TX)** | **~390 mA ≈ 1,95 W** | kurz (ms), bei jeder Sendung |

## Bewertung
- **5 V / 1 A (5 W) Netzteil:** ~4× Reserve im Mittel, ~2,5× im Peak → **komfortabel**. Stromversorgung als
  Dauer-Fehlerquelle damit **ausgeschlossen**, sofern ≥ 1 A.
- **5 V / 0,5 A:** im Mittel ok, aber bei WLAN-TX-Peaks + schwachem Kabel **Brownout-Risiko** → nicht empfohlen.
- **Empfehlung:** 5 V ≥ 1 A, gutes (kurzes/dickes) USB-Kabel, **470–1000 µF Stützkondensator an VIN** gegen die
  WLAN-TX-Peaks. Damit ist die Versorgung kein Risiko mehr.

## Einordnung zur Fehlersuche
Die Boot-Loop-Crashes dieser Sitzung waren **nicht** die Stromversorgung — die Ursache war ein Software-Bug
(globale `TwoWire`-Referenz / SIOF, siehe Firmware-Historie). Das Gerät läuft jetzt stabil über USB.
Das Budget bestätigt: mit ≥ 1 A ist Strom unkritisch.

**Zum echten Ausschluss:** mit einem USB-Strommessgerät (z. B. UM25C) den realen Mittel- und Peak-Strom messen —
dann ist es bewiesen statt geschätzt. Erwartung: Mittel grob 0,2–0,3 A.
