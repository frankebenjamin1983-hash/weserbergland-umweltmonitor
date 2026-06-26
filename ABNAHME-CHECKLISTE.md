# UmweltMonitor — Abnahme-Checkliste (nur auf deiner Maschine)

> Alle statischen Prüfungen sind ausgeschöpft (5 Durchläufe, alle grün).
> Diese drei Schritte beweisen, was statische Prüfung nicht kann.
> Reihenfolge minimiert Kollisionen: n8n zuerst (unabhängig), dann WiFi (überschreibt Gerät).

---

## Schritt 1: n8n-curl-Test (unabhängig, unterbricht nichts)

**Voraussetzung:** Workflow `umweltmonitor_empfang.json` in n8n importiert, Credentials verknüpft, active.

```bash
curl -X POST https://<N8N_HOST>/webhook/umweltmonitor \
  -H "Content-Type: application/json" \
  -d '{"device_id":"umweltmonitor_basis_01","schema_version":1,"timestamp":"2026-06-25T17:37:51Z","seq":0,"soil_moisture_raw":3228,"soil_temp_c":29.38,"water_level_raw":0,"rain":false,"rain_intensity_raw":3907,"air_temp_c":29.27,"humidity_pct":57.6,"dewpoint_c":20.04,"pressure_hpa":1013.4,"uv_raw":0,"lux":166.2,"color_rgb":[478,425,452],"noise_rel":46.2,"gas_raw":null}'
```

**Prüfen in Sheet `kalletal_umweltmonitor` (Ziel 1):**

| # | Spalte | Erwarteter Wert | OK? |
|---|---|---|---|
| 1 | timestamp | ISO-Zeitstempel (n8n, nicht 2026-06-25T17:37:51Z) | ⬜ |
| 2 | device_id | `umweltmonitor_basis_01` | ⬜ |
| 3 | schema_version | `1` | ⬜ |
| 4 | seq | `0` | ⬜ |
| 5 | soil_moisture_raw | `3228` | ⬜ |
| 6 | soil_temp_c | `29.38` | ⬜ |
| 7 | water_level_raw | `0` | ⬜ |
| 8 | rain | `FALSE` | ⬜ |
| 9 | rain_intensity_raw | `3907` | ⬜ |
| 10 | air_temp_c | `29.27` | ⬜ |
| 11 | humidity_pct | `57.6` | ⬜ |
| 12 | dewpoint_c | `20.04` | ⬜ |
| 13 | pressure_hpa | `1013.4` | ⬜ |
| 14 | uv_raw | `0` | ⬜ |
| 15 | lux | `166.2` | ⬜ |
| 16 | color_r | `478` | ⬜ |
| 17 | color_g | `425` | ⬜ |
| 18 | color_b | `452` | ⬜ |
| 19 | noise_rel | `46.2` | ⬜ |
| 20 | gas_raw | LEER / null | ⬜ |

**Prüfen in Sheet `kalletal_umweltmonitor_wetterstation` (Ziel 2):**

| # | Spalte | Erwarteter Wert | OK? |
|---|---|---|---|
| 1 | timestamp | ISO-Zeitstempel (n8n) | ⬜ |
| 2 | temperature | `29.27` (= air_temp_c) | ⬜ |
| 3 | humidity | `57.6` (= humidity_pct) | ⬜ |
| 4 | pressure | `1013.4` (= pressure_hpa) | ⬜ |
| 5 | ref_temperature | `29.38` (= soil_temp_c) | ⬜ |
| 6 | ref_humidity | **LEER** (KEIN Wert!) | ⬜ |
| 7 | sensor_main | `umweltmonitor_basis_01` | ⬜ |
| 8 | sensor_ref | `TMP102` | ⬜ |

**Kritische Negativ-Prüfungen:**

| Prüfung | OK? |
|---|---|
| `dewpoint_c` (20.04) ist NICHT in `ref_humidity` gelandet | ⬜ |
| `dewpoint_c` ist NICHT in Ziel 2 aufgetaucht | ⬜ |
| `gas_raw` = null ist als LEER erhalten, nicht als 0 | ⬜ |
| `rain_intensity_raw` = 3907 ist unverändert (keine Invertierung durch n8n) | ⬜ |

**Bestanden wenn:** Alle 20 + 8 + 4 Checks = 32/32.

---

## Schritt 2: WiFi-Umbau flashen (überschreibt Gerät)

**Voraussetzung:** Kein laufender Powerbank-/Dauerlauf, der unterbrochen werden darf.

```bash
cd firmware/umweltmonitor_basis
arduino-cli compile --fqbn esp32:esp32:esp32 --warnings all .
arduino-cli upload -p COMx --fqbn esp32:esp32:esp32 .
```

**Prüfen nach Upload:**
- Serielle Konsole zeigt Boot-Log: CK1/CK2/CK3, WLAN-IP, NTP-Sync
- SSE-Stream im Panel sichtbar (N:/L:-Zeilen)

---

## Schritt 3: Drop-Test (beweist WiFi-Robustheit)

**Voraussetzung:** Gerät läuft mit WiFi-v2-Firmware, Panel via `http://umweltmonitor.local` verbunden, Serielle Konsole offen.

### Test A — Einfacher Drop (gleiche IP)

| Messung | Wert |
|---|---|
| IP vor Drop | _________ |
| Router aus → `WLAN-WEG` im Log | ___ s |
| Router an → `WLAN-IP` im Log | ___ s |
| IP nach Reconnect | _________ |
| Erster N:/L:-Datenpunkt im Panel | ___ s |
| Panel-Reload nötig? | ja / nein |
| **Bestanden?** | ✅ / ❌ |

### Test B — Drop mit IP-Wechsel (Härtefall)

| Messung | Wert |
|---|---|
| IP vor Drop | _________ |
| DHCP-Lease abgelaufen? (5+ min oder manuell) | ja / nein |
| IP nach Reconnect | _________ |
| IP gewechselt? | ja / nein |
| `mDNS reaktiviert` im Log? | ja / nein |
| `umweltmonitor.local` erreichbar? | ja / nein |
| Panel-Reload nötig? | ja / nein |
| **Bestanden?** | ✅ / ❌ |

**Bestanden wenn beide Tests:** Stream erscheint im Panel ohne Nutzer-Aktion, keine manuelle IP-Eingabe nötig.

**Nicht bestanden wenn:** Panel zeigt dauerhaft „Verbindung verloren", erfordert Reload oder manuelle IP.

---

## Reihenfolge (begründet)

```
1. n8n-curl-Test ← zuerst: unabhängig, unterbricht nichts, schließt Mapping-Beweislücke
2. WiFi flashen  ← danach: überschreibt Gerät, also erst wenn n8n grün ist
3. Drop-Test     ← zuletzt: braucht geflashtes Gerät + Router-Eingriff
```
