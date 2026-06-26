# UmweltMonitor — Abnahme-Stand (2026-06-25)

## Abnahmekriterien
| # | Kriterium | Status | Beleg |
|---|-----------|--------|-------|
| A1 | echte JSON-Zeile vom ESP32 | belegt | COM11, 2 reale Zeilen, 18 Felder, echter Zeitstempel 2026-06-25T17:37:51Z |
| A2 | erfolgreicher Web-Serial-Empfang | offen | einmaliger Browser-Klick (Web-Serial-Freigabe) nötig; Empfang über WLAN bereits gezeigt |
| A3 | Record-Zähler steigt 1×/min | teils | Panel-Records steigen +1/min belegt (#1→#2→#3, exakt 60 s: 17:44:01/45:01/46:01). Geräte-`seq` bleibt 0 = korrekt, weil der signierte POST (Pfad A) noch nicht läuft |
| A4 | Feldzuordnung ohne Validierungsfehler | belegt | echte Zeile durch Panel-Parser: 1 Record, alle Felder, gas_raw=null erhalten, 0 verworfen |

## Echte JSON-Zeilen vom Gerät (COM11)
```
{"device_id":"umweltmonitor_basis_01","schema_version":1,"timestamp":"2026-06-25T17:37:51Z","seq":0,"soil_moisture_raw":3228,"soil_temp_c":29.38,"water_level_raw":0,"rain":false,"rain_intensity_raw":3907,"air_temp_c":29.27,"humidity_pct":57.6,"dewpoint_c":20.04,"pressure_hpa":1013.4,"uv_raw":0,"lux":166.2,"color_rgb":[478,425,452],"noise_rel":46.2,"gas_raw":null}
{"device_id":"umweltmonitor_basis_01","schema_version":1,"timestamp":"2026-06-25T17:38:51Z","seq":0,...}
```

## Record-Takt (Serial-Mitschnitt, 135 s)
```
Record #1  @ +  8s   device-ts=2026-06-25T17:44:01Z  seq=0
Record #2  @ + 68s   device-ts=2026-06-25T17:45:01Z  seq=0
Record #3  @ +128s   device-ts=2026-06-25T17:46:01Z  seq=0
```

## Was erledigt / was offen
- ERLEDIGT: Firmware (WLAN-SSE-Stream), Panel (Web-Serial + WLAN), echtes ISRG-Root-X1-CA eingesetzt, kompiliert sauber (86 % Flash).
- OFFEN für `seq`-steigt (echter Sendepfad / Pfad A):
  1. n8n-Empfänger (Webhook + HMAC-Prüfung) bauen — Zugriff auf Produktivserver vom System gesperrt, braucht Freigabe.
  2. url + hmac aufs Gerät provisionieren + flashen — Gerät muss dafür am USB hängen.
- OFFEN A2: einmaliger Web-Serial-Klick im Edge (Browser-Sicherheit, nicht automatisierbar).

## Gerät
- CH340-Port wechselt bei Replug (war COM8, COM9, zuletzt COM11) — im Web-Serial-Dialog den Eintrag „USB-SERIAL CH340" wählen, Nummer egal.
- WLAN: umweltmonitor.local bzw. zuletzt 192.168.1.245.
