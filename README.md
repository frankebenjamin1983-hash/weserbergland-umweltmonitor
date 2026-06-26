# Umweltmonitor (Kalletal-Erder) — WetterStudioTeam

> 🚧 **Work in Progress** — aktiver Aufbau & Dauertest. Pinbelegung, Datenvertrag und Schnittstellen können sich noch ändern. „Fertig" gilt nur, was im Testplan mit Beleg abgehakt ist (`bewiesen vs. vermutet`).

ESP32-Umweltmonitor für den Außeneinsatz. Eigene Pipeline, die später denselben
Datenfluss wie die bestehende Wetterstation „Erder" speist. **EM-Sonden / Elektrosmog
sind ein separates Schwesterprojekt** (eigener Ordner, hier NICHT enthalten).

Dieser Ordner enthält die **self-contained Prompts** für je einen frischen Chat
(Firmware bzw. Gehäuse/Standort) plus die hier festgelegten Entscheidungen.

---

## Entschieden (Design / Bauteile, Stand 2026-06-24) — NICHT eigenmächtig ändern

### MCU & Analog-Frontend
- **MCU: ESP32-D (WROOM-32, WiFi + Bluetooth)** — vom Nutzer gewählt (2026-06-24): BT (Classic+BLE)
  verfügbar, Dual-Core, **8 ADC1-Kanäle (mit WLAN nutzbar)**. **Bewusste Abweichung** von der
  C3-Stations-Basis. Folgen: mehr Strom/Wärme als C3; WiFi+BT-Coexistence belastet Funk/RAM
  (für die reine Datenübertragung reicht WLAN → BT nur für lokale Konfig/Readout).
- **Analog-Frontend: ADS1115 jetzt OPTIONAL** — der ESP32-D hat genug native ADC1-Kanäle für alle
  Analogsensoren. Siehe [`firmware/belegungsplan-und-entwurf.md`](firmware/belegungsplan-und-entwurf.md):
  - **Variante A (nativ, empfohlen):** alle Analogsensoren am ESP32-ADC1, kein ADS1115.
  - **Variante B (hybrid):** ADS1115 nur für GUVA/MiCS (16-bit + PGA, kleine Signale feiner).
  - Entscheidung offen.

### Architektur & Sensorbestückung
**Konsolidiert (2026-06-24):** ein ESP32-D trägt **Stations-Sensoren + Umweltsensoren** und
ersetzt den alten C3. Die **Stations-Sensoren sind unten noch NICHT erfasst** (Liste/Pins offen
→ Belegungsplan/Pinout daher noch unvollständig). Der Datenvertrag führt beide Feldsätze zusammen.

**Stations-Sensoren (übernommen, ein Board ersetzt den C3):** AHT20 (Temp/Feuchte, I²C 0x38) ·
BMP280 (Druck, I²C **0x77** — verifiziert, nicht 0x76) · LCD16x2 (I²C 0x27, optional — „Display entfernt"). *(DHT22 aus der
AntiWetter-README ist im aktuellen Build nicht enthalten.)*
→ Frost-Sperre des Servos nutzt die AHT20-Temperatur. **Eigenerwärmung wieder relevant** (Board misst
Lufttemp/-feuchte und der ESP32-D heizt mehr als der C3).

**I²C-Aufteilung:** Bus 0 (GPIO21/22) = AHT20 / BMP280 / TCS34725 / LCD; **Bus 1 (GPIO18/19) = TSL2591**
(0x29-Konflikt gelöst, Entscheidung (a)).

Umweltsensoren (zusätzlich zur Station, fix):
| Sensor | Messgröße | Bus / ADC-Weg | Einheit | Vorbehalt |
|---|---|---|---|---|
| HW-390 (kapazitiv v2.0) | Bodenfeuchte | analog → ADS1115 | % rel. | Ausgang spannungsabhängig → stabile 3,3 V / ratiometrisch, kalibrieren |
| HW-038 | Wasserstand/Nässe | analog → ADS1115 | rel. Pegel | Leitfähigkeitstyp → **korrodiert bei Dauerstrom** → nur zur Messung bestromen |
| Raindrop (LM393, AO+DO) | Regen jetzt + Intensität | digital DO an GPIO (alt. AO) | Index | **kein** mm; korrodiert → nur zur Messung bestromen |
| MAX9814 | Schallpegel | analog → **native C3-ADC** | rel. dB | Audio-Wellenform → schnelles Sampling + RMS; dB(A) nur kalibriert |
| MiCS-5524 | Gas (CO / brennbar / VOC) | analog → ADS1115 | rel. | MOS + **Heizer (Stromfresser + Warmup)**; nur indikativ, kalibrieren |
| GUVA-S12SD | UV | analog → ADS1115 | UV-Index n. Kennlinie | Modulausgang ~0–1 V (klein) |
| TCS34725 | Farbe (RGB+Clear) | I²C (0x29) | RGB | kein Präzisions-Lux, kein UV |
| TSL2591 | Helligkeit (Lux, HDR) | I²C (0x29) | Lux | **Adresskonflikt 0x29 mit TCS34725** → 2. I²C-Bus / Mux / eins weglassen |

### Aktorik
- **Servo SG90 9g** zum Leeren des Regenbehälters → macht **Regenmenge (mm) messbar**:
  `mm = (Anzahl Leerungen × Dump-Volumen) / Fangfläche` (1 mm = 1 L/m²), **nach Kalibrierung**
  von Fangfläche und Dump-Volumen. Raindrop-Sensor bleibt daneben (Sofort-Detektion).
- 1× PWM-GPIO (ESP32 LEDC). **Eigene 5-V-Versorgung + Stützkondensator + gemeinsame Masse**
  (Anlauf-/Stall-Strom ~0,5–0,7 A → sonst Brownout). PWM mit 3,3-V-Logik geht praktisch.
- SG90 **nicht wetterfest** → schützen oder wasserfeste MG90-Variante; Mechanik balanciert
  auslegen (geringes Drehmoment ~1,8 kg·cm); **Frost-Sperre** in der Firmware.

### Funk
- **ESP-NOW** ist erste Wahl für verteilte Sensor-Nodes (peer-to-peer, ohne Router,
  stromsparend, läuft auf C3) → Gateway-ESP32 sammelt + POSTet an n8n.
- **WiFi** nur, wenn jeder Knoten autark direkt an n8n senden soll (höchster Verbrauch).
- **BLE** nur für gelegentliches Handy-Auslesen/Provisioning. **BT Classic: nein.**
- Offen: ein verkabelter Aufbau vs. verteilte Nodes — entscheidet der Gehäuse/Standort-Chat
  (Platzkonflikte: UV→Himmel, Mic/Gas→freie Luft, Boden/Wasser/Regen→Erdboden).

### Datenweg (von der Wetterstation geerbt — 1:1 übernehmen, nicht ändern)
- ESP32-C3, Messintervall ~10 min → POST an n8n `https://<N8N_HOST>`.
- Google Sheet „<SHEET_DOC>", Tab der Station: `<SHEET_TAB>`.
- Live-Webhook `…/webhook/wetter-latest`; bestehende Android-/Windows-Widgets.
- **Stolperfallen der Station:** n8n übernimmt Änderungen nur per UI-„Save" (direkte
  DB-Edits werden ignoriert); das Sheets-Secret = das Gmail-Secret.
- **Offen:** selbes Sheet (eigener Tab) vs. eigenes Sheet + eigener Webhook — entscheidet
  der Firmware-Chat.

---

## Noch zu verifizieren (Messung / Kalibrierung) — NICHT als bewiesen behandeln
Diese Punkte sind **Annahmen/Designvorgaben, kein Messbeweis** — erst am lebenden System bzw.
nach Kalibrierung bestätigen (Prinzip „fertig = verifiziert"):
- **MiCS-5524 misst CO / brennbare Gase / VOC — KEIN CO₂, kein NDIR.** Im Datensatz **nicht**
  „co2" nennen → `gas_raw` (relativ, kalibrierpflichtig). Es gibt **keinen** CO₂-Sensor im Set.
- **Schall = relativer Pegel**, nicht dB SPL/dB(A) → `sound_level_rel`, nicht `db_spl`.
- **Bodenfeuchte, UV-Index, Regen-mm** erst nach Kalibrierung absolut; vorher `*_raw/_rel`.
- **Belegungsplan/Pins** hängen am realen C3-Board (ADC2 bei WLAN gesperrt → native Analogpins
  nur ADC1) → konkrete GPIO-Nummern erst nach Board-Bestätigung.

---

## Dateien / Prompts
- [`firmware/belegungsplan-und-entwurf.md`](firmware/belegungsplan-und-entwurf.md) — Belegungsplan (ESP32-D) + Firmware-Entwurf.
- [`firmware/STUECKLISTE.md`](firmware/STUECKLISTE.md) — Stückliste (BOM) mit Pins & I²C-Adressen.
- [`pinout.txt`](pinout.txt) — kompakte Pinbelegung.
- [`firmware/logik-flowchart.md`](firmware/logik-flowchart.md) — Logik-Flussdiagramm (Mermaid: Sensoren → Streams/SSE/POST → Panel/n8n).
- [`panel/umweltmonitor_panel.html`](panel/umweltmonitor_panel.html) — Live-Daten-Panel (Web Serial + WLAN).
- [`firmware/TESTPLAN.md`](firmware/TESTPLAN.md) — Testplan (Phase 0): Erfolgssignale je Kanal/Servo/Datenweg.
- [`prompts/firmware-prompt.md`](prompts/firmware-prompt.md) — Firmware-Prompt für frischen Chat (⚠ nennt noch ESP32-C3 → vor Nutzung auf ESP32-D + Belegungsplan-Variante syncen).
- [`prompts/gehaeuse-standort-prompt.md`](prompts/gehaeuse-standort-prompt.md) — Gehäuse/Standort-Prompt (⚠ nennt noch ESP32-C3).

Jeder Prompt ist self-contained für einen frischen Chat. Beide verlangen: **Testplan
(`TESTPLAN.md`) vor dem Tun**, **bewiesen vs. vermutet strikt trennen**, festgelegte
Werte nicht eigenmächtig ändern, Dateien unter diesem Projektpfad ablegen.

**Doku-Status (2026-06-24):** README + Belegungsplan auf **ESP32-D** aktuell; TESTPLAN angelegt
(alle Einträge offen). Die beiden Prompts referenzieren noch ESP32-C3 → werden synchronisiert,
sobald die offenen Entscheidungen (ADS1115 A/B, Frost-Temp-Quelle, BT-Rolle, Deep-Sleep,
Zeitstempel) getroffen sind.
