# UmweltMonitor — Abnahme-Stand (2026-06-27)

## Abnahmekriterien

| # | Kriterium | Status | Beleg |
|---|-----------|--------|-------|
| A1 | Echte JSON-Zeile vom ESP32 | ✓ belegt | COM8, echte Zeile, 18 Felder, Zeitstempel korrekt (2026-06-25) |
| A2 | Web-Serial-Empfang im Panel | teils | WLAN-SSE belegt; einmaliger Browser-Klick für Web-Serial (Sicherheitsfreigabe) nötig |
| A3 | Record-Zähler steigt 1×/min | ✓ belegt | Panel-Records +1/min: #1→#2→#3, exakt 60 s Takt |
| A4 | Feldzuordnung ohne Validierungsfehler | ✓ belegt | Panel-Parser: alle Felder korrekt, gas_raw=null erhalten |
| A5 | WLAN-Watchdog aktiv | ✓ belegt | Code aktiv, Reset nach 5 min offline, Reset-Grund wird geloggt |
| A6 | FRAM seq-Persistenz | ✓ belegt | FRAM 0x50 erkannt, framLoadSeq/saveSeq aktiv, Magic 0xDEADBEEF |
| A7 | TFT-Display aktiv | ✓ belegt | Display zeigt Temp/Feuchte/Lux/UV/WLAN-Status (Foto 2026-06-27) |
| A8 | n8n Pfad A (seq steigt) | offen | Secrets-Provisioning + SSH-Zugang zu 85.9.202.196 nötig |
| A9 | Dauertest 24h ohne Ausfall | offen | steht aus |

## Aktueller Firmware-Stand

- **Flash:** `83f84b4` (2026-06-27) — Stufe 1b+2 aktiv
- **Bibliotheken:** Adafruit FRAM I2C 2.0.3, Adafruit ST7735/ST7789 1.11.0
- **Reset-Grund-Log:** aktiv seit erstem Watchdog-Flash
- **Betrieb:** autonom (kein USB), Starlink-Uplink, n8n-Executions grün bis 07:14

## Offene Punkte

1. **Pfad A (HTTPS-POST):** n8n-Webhook + HMAC-Prüfung einrichten → Secrets in NVS provisionieren. Braucht SSH-Freigabe für 85.9.202.196.
2. **Dauertest:** 24h+ laufen lassen, Reset-Grund nach nächstem Ausfall auslesen.
3. **Kalibrierung:** Bodenfeuchte (trocken/nass-Referenz), UV-Index (Kennlinie GUVA), Gas (CO-Referenz), Schall (dB-Referenz).
4. **Brownout-Diagnose:** 1000µF direkt an VIN/GND installiert (2026-06-27). Nächster Ausfall zeigt ob POWERON oder BROWNOUT.

## Gerät

- **Port:** COM8 (CH340 — wechselt bei Replug, im Web-Serial-Dialog „USB-SERIAL CH340" wählen)
- **WLAN:** `umweltmonitor.local` / SSE: `http://umweltmonitor.local/`
- **Versorgung:** 5-V-Netzteil, 1000µF direkt an VIN/GND

## Verifizierte JSON-Beispielzeile (2026-06-25)

```json
{"device_id":"umweltmonitor_basis_01","schema_version":1,"timestamp":"2026-06-25T17:37:51Z","seq":0,"soil_moisture_raw":3228,"soil_temp_c":29.38,"water_level_raw":0,"rain":false,"rain_intensity_raw":3907,"air_temp_c":29.27,"humidity_pct":57.6,"dewpoint_c":20.04,"pressure_hpa":1013.4,"uv_raw":0,"lux":166.2,"color_rgb":[478,425,452],"noise_rel":46.2,"gas_raw":null}
```
