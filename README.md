# UmweltMonitor — Weserbergland

ESP32-Außenumweltmonitor. Sendet Messdaten per HTTPS an n8n → Google Sheet,
streamt live per SSE (WLAN) und USB-Serial an das Web-Panel.

Stand: 2026-06-27 — Gerät läuft autonom, alle Stufen aktiv.

---

## Hardware

| Bauteil | Funktion | Anschluss |
|---------|----------|-----------|
| ESP32-D0WD NodeMCU (30-Pin) | MCU + WLAN | — |
| AHT20 + BMP280 (Combo) | Temp / Feuchte / Druck | I²C-0 0x38 + 0x77 |
| TCS34725 | Farbe / RGB | I²C-0 0x29 |
| TSL2591 | Lux HDR | I²C-1 0x29 |
| TMP102 | Bodentemperatur | I²C-0 0x48 |
| MB85RC256V FRAM | Persistenz (seq, Ringpuffer) | I²C-0 0x50 |
| ST7789 1.47" TFT | Display (Temp/Feuchte/Lux/UV/Status) | SPI-HSPI |
| HW-390 | Bodenfeuchte | GPIO36 ADC1 |
| HW-038 | Wasserstand / Nässe | GPIO39 ADC1 |
| Raindrop (LM393) | Regen DO + AO | GPIO27 / GPIO33 |
| MAX9814 | Schallpegel | GPIO34 ADC1 |
| MiCS-5524 | Gas (CO / brennbar) | GPIO35 ADC1 + GPIO26 EN |
| GUVA-S12SD | UV | GPIO32 ADC1 |

Versorgung: 5 V an VIN. Alle Sensoren = Breakout-Module (Pull-ups intern).

→ Vollständige Pinbelegung: [`pinout.txt`](pinout.txt)
→ Stückliste mit I²C-Adressen: [`firmware/STUECKLISTE.md`](firmware/STUECKLISTE.md)

---

## Firmware-Architektur

**Zwei Datenwege parallel:**

- **Pfad A — signierter HTTPS-POST → n8n (1×/min):**
  HMAC-SHA256 über den JSON-Body, CA-geprüftes HTTPS (ISRG Root X1).
  `seq` steigt nur bei HTTP 2xx (monoton, Replay-Schutz).
  Secrets in NVS provisioniert.

- **Pfad B — lokaler Live-Stream (Echtzeit):**
  USB-Serial + WLAN-SSE (Port 80, `umweltmonitor.local`).
  `N:<rms>` ~5 Hz (Schall), `L:<lux>,<uv>,<r>,<g>,<b>,<dV>` ~2 Hz, JSON 1×/min.
  Ungesichert — nur für lokales WLAN.

**Persistenz:**
- FRAM (MB85RC256V): seq-Zähler + Magic 0xDEADBEEF. Überlebt Reboot ohne NVS-Schreibzyklus.
- NVS (Preferences): Fallback wenn kein FRAM.

**WLAN-Watchdog:**
Prüft alle 30 s. Nach 5 min ohne WLAN → `ESP.restart()`.
Disconnect-Grund wird geloggt (Grund-Code + Klartext).

**Reset-Grund-Logging:**
`esp_reset_reason()` bei jedem Boot → Serial: POWERON / BROWNOUT / PANIC / WATCHDOG etc.

**Display (ST7789 TFT):**
Aktualisierung 1×/min nach jedem Messzyklus.
Zeigt: Temp · Feuchte · Taupunkt · Druck · Lux · UV · dV (Lichtdynamik) · FRAM-Status · WLAN+RSSI · Uptime.

→ Logik-Flussdiagramm: [`firmware/logik-flowchart.md`](firmware/logik-flowchart.md)

---

## Pinout (Kurzform)

```
I²C-0 (GPIO21/22): AHT20 · BMP280 · TCS34725 · TMP102 · FRAM
I²C-1 (GPIO18/19): TSL2591
SPI-HSPI TFT:  SCK=14  MOSI=13  CS=25  DC=16  RST=4  BL=17
ADC1: GPIO36 Boden · GPIO39 Wasser · GPIO34 Schall · GPIO35 Gas · GPIO32 UV · GPIO33 Regen
Digital: GPIO27 Regen-DO · GPIO26 MiCS-EN
Strapping-Pins gemieden: GPIO2 (→DC=16), GPIO15 (→CS=25)
```

---

## Datenvertrag (JSON, 1×/min)

```json
{
  "device_id": "umweltmonitor_basis_01",
  "schema_version": 1,
  "timestamp": "2026-06-27T...",
  "seq": 847,
  "soil_moisture_raw": 2100,
  "soil_temp_c": 21.5,
  "water_level_raw": 0,
  "rain": false,
  "rain_intensity_raw": 3900,
  "air_temp_c": 22.3,
  "humidity_pct": 58.1,
  "dewpoint_c": 13.9,
  "pressure_hpa": 1013.2,
  "uv_raw": 0,
  "lux": 496.3,
  "color_rgb": [1172, 894, 833],
  "noise_rel": 42.1,
  "gas_raw": null
}
```

`gas_raw=null` die ersten 3 min (MiCS-Warmup). Alle Werte ohne Kalibrierung = relativ/raw.

---

## Sicherheit

- Secrets (SSID, Pass, HMAC-Key, n8n-URL) ausschließlich in NVS — nie im Code.
- n8n-Endpunkt in Doku redaktiert: `<N8N_HOST>`.
- Sheet-Namen redaktiert: `<SHEET_DOC>` / `<SHEET_TAB>`.
- HTTPS CA-geprüft (ISRG Root X1, kein `setInsecure()`).

→ Details: [`SICHERHEITSARCHITEKTUR.md`](SICHERHEITSARCHITEKTUR.md)

---

## Dateien

| Datei | Inhalt |
|-------|--------|
| [`pinout.txt`](pinout.txt) | Vollständige GPIO-Belegung |
| [`firmware/STUECKLISTE.md`](firmware/STUECKLISTE.md) | BOM mit I²C-Adressen und Pins |
| [`firmware/logik-flowchart.md`](firmware/logik-flowchart.md) | Mermaid-Flussdiagramm |
| [`firmware/umweltmonitor_basis/`](firmware/umweltmonitor_basis/) | Arduino-Sketch |
| [`panel/umweltmonitor_panel.html`](panel/umweltmonitor_panel.html) | Web-Panel (Serial + WLAN-SSE) |
| [`firmware/TESTPLAN.md`](firmware/TESTPLAN.md) | Testplan mit Abnahmekriterien |
| [`ABNAHME-STAND.md`](ABNAHME-STAND.md) | Aktueller Verifikationsstand |
| [`SICHERHEITSARCHITEKTUR.md`](SICHERHEITSARCHITEKTUR.md) | Sicherheitskonzept |
| [`leistungsbudget.md`](leistungsbudget.md) | Stromverbrauchsschätzung |

---

## Status

| Komponente | Status |
|-----------|--------|
| Firmware kompiliert + geflasht | ✓ |
| WLAN-Verbindung + SSE-Stream | ✓ |
| Alle Sensoren aktiv (7×) | ✓ |
| FRAM aktiv (seq persistent) | ✓ |
| TFT-Display aktiv | ✓ |
| WLAN-Watchdog + Reset-Log | ✓ |
| n8n Pfad A (HTTPS-POST) | offen — Secrets-Provisioning + SSH-Freigabe nötig |
| Dauertest 24h | offen |

> **Bewiesen ≠ vermutet.** Alles ohne Beleg in der Tabelle = Vermutung oder Design-Annahme.
> Kalibrierung (Bodenfeuchte, UV-Index, Gas, Schall-dB) steht noch aus.
