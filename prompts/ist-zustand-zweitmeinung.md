# Ist-Zustand-Prompt — Zweitmeinung Umweltmonitor (Stand 2026-06-25)

> Self-contained zum Kopieren an einen unabhängigen Reviewer. Gestützt auf den realen Datei-Stand
> (`anschlussliste.md`, `netlist-kicad.md`, `umweltmonitor_basis.ino`).

---

**Zweitmeinung gesucht: kritisches Review eines ESP32-Umweltmonitors (Ist-Zustand)**

Bitte gib mir eine unabhängige **zweite Meinung**. Du hast keinen Vorkontext — alles Nötige steht hier. Sag mir ehrlich, was **falsch, riskant oder suboptimal** ist und wo du es anders machen würdest. Trenne **bewiesen vs. vermutet** und erfinde keine Bauteile/Features.

**Ziel:** Fest installierter Umwelt-Sensorknoten an einem abgelegenen Standort (Weserbergland), Netzbetrieb. Misst periodisch und sendet an eine bestehende n8n-Pipeline (HTTPS → Google Sheet). Eine separate Wetterstation (ESP32-C3) existiert bereits; dieser „Umweltmonitor" ist ein eigenes Gerät.

**Hardware (bestätigt, alles fertige Breakout-Module):**
- MCU: ESP32-D0WD NodeMCU, 30-Pin; Versorgung 5 V an VIN.
- Analog (ADC1): HW-390 Bodenfeuchte (IO36), HW-038 Wasserstand (IO39), MAX9814 Schall (IO34), MiCS-5524 Gas (IO35, über 10k/10k-Teiler), GUVA-S12SD UV (IO32), Raindrop-Intensität AO (IO33).
- Digital: Raindrop DO (IO27, in), MiCS-EN (IO26, out, dauerhaft HIGH = Heizer an).
- I²C-Bus 0 (SDA IO21 / SCL IO22): AHT20 (0x38) + BMP280 (0x76) als Combo-Board, TCS34725 (0x29), TMP102 Bodentemp (0x48, gekabelt).
- I²C-Bus 1 (SDA IO18 / SCL IO19): TSL2591 Lux (0x29) — getrennt wegen 0x29-Konflikt mit TCS34725.
- Keine externen Widerstände außer dem MiCS-AO-Teiler (I²C-Pull-ups sind auf den Modulen).
- Frei: IO4/IO13/IO14/IO16/IO17/IO23/IO25. TX0/RX0 = USB-Seriell (frei). Strapping IO2/5/12/15 gemieden.

**Bewusste Entscheidungen (Kontext kennen, gern hinterfragen):**
- Klassischer ESP32-D statt C3 (mehr ADC-Kanäle/BT; akzeptiert: mehr Wärme).
- Kein MOSFET-Gating: HW-038 + Raindrop dauerbestromt an 3V3. Wichtig: Abdichtung der Elektronik löst die *Sonden*-Korrosion (elektrolytisch, an der bestromten Sonde in Nässe) **nicht**. Reale Optionen offen: (a) IO14 + MOSFET-Gating oder (b) Verschleißteil mit Tauschintervall.
- Kein Servo/Regen-Entleerung, kein DS18B20, kein CO₂/eCO₂/VOC-Sensor (Gas nur relativer MiCS-Trend).
- Bodentemperatur = TMP102 (I²C), nicht 1-Wire.
- Netzbetrieb, kein Deep-Sleep, keine SD-Karte.

**Stromversorgung, Einsatzort & Mechanik (großteils noch offen):**
- Versorgung: 5-V-Netzteil → VIN; genauer Typ und Strombudget noch nicht gemessen/festgelegt; kein Akku/Solar.
- Einsatz: dauerhafter Außeneinsatz nahe der Weser — feucht, Winterfrost und sommerliche direkte Sonne möglich. Gehäuse (belüftet für T/H/P, zugleich regengeschützt) noch nicht ausgeführt.
- Sensorpositionen offen; aktuell keine thermische Entkopplung zwischen ESP32 und Luftsensor (AHT20/BMP280) → Eigenerwärmungs-Bias ~+1…3 °C realistisch. Boden-Sensoren (HW-390, TMP102) gekabelt im Erdreich, Verguss nötig.
- Analog-Strategie: alle Analogsensoren über die internen ESP32-ADC (ADC1), kein externer ADC. ESP32-ADC ist nichtlinear, verrauscht, Referenz streut → ADC-Kalibrierung/Kennlinienkorrektur noch offen.

**Datenweg & Sicherheit:**
- ~1-min-Zyklus → HTTPS-POST an n8n (Caddy + Let's Encrypt).
- Payload = JSON mit device_id, schema_version, ISO-Timestamp, seq und den Messwerten; unkalibrierte Größen als `*_raw/_rel`; kein co2/voc.
- Authentizität/Integrität: HMAC-SHA256 (ESP32-mbedTLS) über den exakten Body inkl. seq+timestamp, im Header `X-Signature`. Replay-Schutz: monotoner `seq`, nur bei erfolgreichem POST hochgezählt, in NVS persistiert.
- HTTPS mit Zertifikatsprüfung: aktuell Verifikation gegen hinterlegte Root-CA (Let's Encrypt ISRG Root X1), kein `setInsecure`. Art des Pinnings (Root-CA vs. Intermediate vs. Public-Key) noch nicht final entschieden. Secrets (WLAN, URL, HMAC-Key, CA) in NVS, nicht im Code. Watchdog + Brownout aktiv.
- Zeitquelle: NTP über WLAN; kein RTC-Baustein. Bei NTP-Ausfall wird `timestamp` als „unsynced" markiert.

**Firmware (Arduino/ESP32, Entwurf v1 — noch NICHT kompiliert/auf Hardware getestet):**
- Zwei `TwoWire`-Instanzen (Wire/Wire1). Adafruit-Libs für AHT20/BMP280/TCS34725/TSL2591; TMP102 direkt per Registerlesung.
- Schall = Single-Pass AC-RMS. Gas erst nach ~3 min Warmup, sonst `null`. Taupunkt `dewpoint_c` on-device (Magnus, invariant gegen die Eigenerwärmung).
- JSON-Beispiel:
```json
{"device_id":"umweltmonitor_basis_01","schema_version":1,"timestamp":"2026-06-25T12:00:00Z","seq":42,
 "soil_moisture_raw":0,"soil_temp_c":0.0,"water_level_raw":0,"rain":false,"rain_intensity_raw":0,
 "air_temp_c":0.0,"humidity_pct":0.0,"dewpoint_c":0.0,"pressure_hpa":0.0,"uv_raw":0,"lux":0.0,
 "color_rgb":[0,0,0],"noise_rel":0.0,"gas_raw":0}
```

**Bekannt offen / TODO (ehrlich):**
- LE-Root-CA noch Platzhalter → POST scheitert bis echtes Zertifikat eingesetzt ist.
- Secrets-Provisioning (NVS) noch nicht implementiert.
- Kalibrierung fehlt: Bodenfeuchte→VWC, UV→Index, Schall→dB(A), Gas (alle aktuell relativ/raw).
- MiCS-EN-Polarität und Raindrop-DO-Logik (LOW=nass?) am realen Modul prüfen.
- MiCS-AO-Teiler nur falls AO >3,3 V — nachmessen.
- ESP32-Core-Kompatibilität: TSL2591 auf 2. I²C-Bus, JSON-Puffer geprüft.
- TMP102 nicht wasserdicht (Bodeneinsatz → vergießen).
- Korrosion HW-038/Raindrop (Gating vs. Verschleißteil) noch zu entscheiden.
- Thermische Entkopplung + Gehäusebelüftung, Strombudget/Wärme im Dauerbetrieb offen.
- Alle Verifikationstests (Sensorkanäle, TLS/HMAC/Replay-Negativtests) noch offen — Hardware nicht aufgebaut.

**Bitte um Zweitmeinung — konkret:**
1. Hardware/Verdrahtung: Fehler, Risiken, bessere Pin-/Bus-Wahl? ADC1-Belegung korrekt (ADC2 bei WLAN gesperrt)?
2. Firmware: Bugs, Bibliotheks-/Core-Fallstricke (2. I²C-Bus, Watchdog-API, RMS, JSON-Puffer, BMP280/AHT20-Adressen)?
3. Sicherheit: ist HMAC + seq + TLS-Pinning sinnvoll/ausreichend? Schwachstellen, bessere Praxis?
4. Architektur/Sinn: passt der Sensorsatz zum Ziel? Was weglassen/ergänzen? Korrosion bei HW-038/Raindrop ohne Gating — tragbar?
5. Was übersiehst du als unabhängiger Reviewer, das mir entgangen sein könnte?
6. Welche drei Risiken würdest du vor dem Hardwareaufbau zuerst eliminieren?
7. Welche drei Messwerte des Systems liefern voraussichtlich den höchsten praktischen Nutzen?
8. Welche Teile wirken überkompliziert / erzeugen mehr Aufwand als Erkenntnis?
