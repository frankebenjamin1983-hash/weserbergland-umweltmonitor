# UmweltMonitor → n8n: Empfänger-Workflow

> Eigenständiger Workflow. Wetterstation unverändert (Diff = 0).

## Architektur

```
ESP32 UmweltMonitor
       │  POST /umweltmonitor  (JSON: 18 Felder)
       ▼
┌──────────────────────┐
│  Webhook (POST)      │
└──────┬───────────────┘
       │  (parallel)
       ├─────────────────────────────────────┐
       ▼                                     ▼
┌──────────────────────┐          ┌──────────────────────┐
│ Mapping: Vollständig  │          │ Mapping: Kompatibel   │
│ 20 Spalten            │          │  8 Spalten            │
│ (timestamp + 18       │          │ (Wetterstation-Format)│
│  Felder + color_rgb   │          │ ref_humidity = LEER   │
│  gesplittet in r/g/b) │          │ dewpoint_c fehlt      │
└──────┬───────────────┘          └──────┬───────────────┘
       ▼                                     ▼
┌──────────────────────┐          ┌──────────────────────┐
│ Ziel 1:              │          │ Ziel 2:              │
│ kalletal_            │          │ kalletal_            │
│ umweltmonitor        │          │ umweltmonitor_       │
│ (vollständig)        │          │ wetterstation        │
└──────────────────────┘          │ (kompatibel)         │
                                  └──────────────────────┘
```

## Mapping: Vollständig (Ziel 1 → kalletal_umweltmonitor)

| Sheet-Spalte | Quelle | Typ |
|---|---|---|
| `timestamp` | `$now.toISO()` (n8n) | string |
| `device_id` | `body.device_id` | string |
| `schema_version` | `body.schema_version` | number |
| `seq` | `body.seq` | number |
| `soil_moisture_raw` | `body.soil_moisture_raw` | number |
| `soil_temp_c` | `body.soil_temp_c` | number |
| `water_level_raw` | `body.water_level_raw` | number |
| `rain` | `body.rain` | boolean |
| `rain_intensity_raw` | `body.rain_intensity_raw` | number |
| `air_temp_c` | `body.air_temp_c` | number |
| `humidity_pct` | `body.humidity_pct` | number |
| `dewpoint_c` | `body.dewpoint_c` | number |
| `pressure_hpa` | `body.pressure_hpa` | number |
| `uv_raw` | `body.uv_raw` | number |
| `lux` | `body.lux` | number |
| `color_r` | `body.color_rgb[0]` | number |
| `color_g` | `body.color_rgb[1]` | number |
| `color_b` | `body.color_rgb[2]` | number |
| `noise_rel` | `body.noise_rel` | number |
| `gas_raw` | `body.gas_raw` | string (null bleibt null) |

## Mapping: Kompatibel (Ziel 2 → kalletal_umweltmonitor_wetterstation)

Identisches Format wie `<SHEET_TAB>`.

| Sheet-Spalte | Quelle | Hinweis |
|---|---|---|
| `timestamp` | `$now.toISO()` (n8n) | Zeichengleich mit Wetterstation |
| `temperature` | `body.air_temp_c` | Kalibriert °C |
| `humidity` | `body.humidity_pct` | Kalibriert % |
| `pressure` | `body.pressure_hpa` | Kalibriert hPa |
| `ref_temperature` | `body.soil_temp_c` | Bodentemperatur °C |
| `ref_humidity` | **LEER** | Kein %-Pendant! dewpoint_c ist °C (Taupunkt), NICHT % |
| `sensor_main` | `body.device_id` | `umweltmonitor_basis_01` |
| `sensor_ref` | `"TMP102"` (fest) | Annahme bis Live-Export klärt |

## Import-Anleitung

### 1. Sheet-Vorbereitung (vor Workflow-Import!)

Im Google Sheet `<SHEET_DOC>` (`1LIMTv3GW8rqSMtCu2pLi0zJ6PCRrdQtvVMSLZsHvlXU`):

**Ziel 1 — Neues Sheet `kalletal_umweltmonitor`:**
Kopfzeile (Zeile 1) MUSS exakt diese 20 Spalten enthalten:
```
timestamp | device_id | schema_version | seq | soil_moisture_raw | soil_temp_c | water_level_raw | rain | rain_intensity_raw | air_temp_c | humidity_pct | dewpoint_c | pressure_hpa | uv_raw | lux | color_r | color_g | color_b | noise_rel | gas_raw
```

**Ziel 2 — Neues Sheet `kalletal_umweltmonitor_wetterstation`:**
Kopfzeile (Zeile 1) MUSS exakt diese 8 Spalten enthalten:
```
timestamp | temperature | humidity | pressure | ref_temperature | ref_humidity | sensor_main | sensor_ref
```

⚠️ **Ohne Kopfzeile greift `autoMapInputData` nicht → Felder fallen still weg.**

### 2. Workflow importieren

1. n8n → Import from File → `umweltmonitor_empfang.json`
2. Google Sheets-Credentials neu verknüpfen (die importierten Nodes zeigen auf `JAcG32sPIOx0c8tU` — nach Import den existierenden Google-Account zuweisen)
3. Webhook-Pfad prüfen: `/umweltmonitor`
4. Workflow **active** schalten

### 3. Test mit echter Gerätezeile

```bash
curl -X POST https://<N8N_HOST>/webhook/umweltmonitor \
  -H "Content-Type: application/json" \
  -d '{"device_id":"umweltmonitor_basis_01","schema_version":1,"timestamp":"2026-06-25T17:37:51Z","seq":0,"soil_moisture_raw":3228,"soil_temp_c":29.38,"water_level_raw":0,"rain":false,"rain_intensity_raw":3907,"air_temp_c":29.27,"humidity_pct":57.6,"dewpoint_c":20.04,"pressure_hpa":1013.4,"uv_raw":0,"lux":166.2,"color_rgb":[478,425,452],"noise_rel":46.2,"gas_raw":null}'
```

Erwartung:
- HTTP 200 `{"status":"ok","timestamp":"..."}`
- Sheet `kalletal_umweltmonitor`: 1 neue Zeile, alle 20 Spalten gefüllt, `gas_raw` = leer/null
- Sheet `kalletal_umweltmonitor_wetterstation`: 1 neue Zeile, 8 Spalten, `ref_humidity` = leer

## Verifikation (Abnahmekriterien)

| # | Kriterium | Status |
|---|---|---|
| 1 | Workflow importiert und active | ⬜ |
| 2 | Wetterstation-Workflow unverändert (Diff = 0) | ⬜ |
| 3 | Echte Gerätezeile: alle 20 Spalten in `kalletal_umweltmonitor` | ⬜ |
| 4 | Echte Gerätezeile: 8 Spalten in `kalletal_umweltmonitor_wetterstation` | ⬜ |
| 5 | `ref_humidity` ist leer (nicht dewpoint_c!) | ⬜ |
| 6 | `dewpoint_c` in eigener Spalte (Ziel 1), NICHT in Ziel 2 | ⬜ |
| 7 | `gas_raw` = null als leer/null erhalten, nicht 0 | ⬜ |
| 8 | `rain_intensity_raw` 4095 = trocken (unverändert durchgereicht) | ⬜ |
| 9 | Kein Feld verloren (18 Quellfelder → 20 Zielspalten inkl. timestamp + color-split) | ⬜ |

## Nicht Teil dieses Workflows

- **HMAC-Prüfung** — separat, sobald Pfad A provisioniert ist (der ESP32 sendet derzeit ohne Signatur)
- **HTTPS/Zertifikatsprüfung** — auf n8n-Server-Ebene (Caddy), nicht im Workflow
- **Replay-Schutz** — separat, sobald seq hochzählt
- **Wetterstation-Workflow** — bleibt VOLLSTÄNDIG UNVERÄNDERT
