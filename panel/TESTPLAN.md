# TESTPLAN — UmweltMonitor Panel (Stufe 1a)

Phase 0: Ziele **vor** dem Code. Fertig = alle Einträge abgehakt **+ am echten Gerät (COM8) verifiziert**.
Erledigtes mit Datum + Beleg abhaken, nicht löschen.

## Spezifikation
Einzel-HTML-Panel liest den UmweltMonitor-Serialstream via **Web Serial @115200**, zeigt Live-Werte
(Ring-Gauges = **kalibriert**, Tiles = **roh**, ehrlich getrennt), hält eine **rollende Zeitreihe** im Speicher
und exportiert sie als **CSV/JSON** (Brücke zum späteren Jupyter-Notebook). Optik kombiniert aus den zwei
Vorbildern (Dark-Ring-Dashboard + Weather-Display-Tiefe). **Stufe 1a: Live + Datenhaltung + Export.**

## Output-Garantien
Parst die reale 18-Feld-JSON-Zeile; roh ≠ kalibriert sichtbar markiert; Export = strukturierte Records mit
Zeitstempel (re-importierbar); `null`/NaN als „—", nicht 0; Nicht-JSON-Zeilen (Banner, `CK*`, `FEHLER`)
werden verworfen, nicht zum Absturz gebracht.

| # | Test | Art | Erfolgssignal | Status |
|---|------|-----|---------------|--------|
| 1 | Web Serial → COM8 verbinden, erste JSON-Zeile | live | alle 18 Felder erscheinen, Werte plausibel | [x] live (Nutzer-Wahrnehmung „sieht ok aus", 2026-06-25) + Preview-Parse-Beleg |
| 2 | Feldnamen gegen echtes JSON | live | kein Feld fehlt/falsch benannt | [x] live (Nutzer-Wahrnehmung 2026-06-25); alle 18 Felder gerendert |
| 3 | roh/kalibriert getrennt | auto | °C/%/hPa/Lux als Ringe; soil/water/rain/uv/noise/gas als **ROH**-Tiles | [x] 2026-06-25 (4 Ringe + 10 Tiles, ROH/KAL-Badges) |
| 4 | Live-Update (~1/min) | live | neue Zeile → Anzeige + Sparkline aktualisiert, `seq` steigt | [x] live (Nutzer-Wahrnehmung 2026-06-25; Dauerlauf über Minuten nicht explizit belegt) |
| 5 | Taupunkt + Spread zentral | auto | `dewpoint_c` + (air−dew) K; Kondens-Hinweis bei kleinem Spread | [x] 2026-06-25 (Spread 1,5 K → Risiko „HOCH") |
| 6 | Rollende Zeitreihe + Export | auto | CSV & JSON enthalten alle Records mit Zeitstempel, re-importierbar | [x] 2026-06-25 (JSON round-trip, recv_iso, 19-Spalten-CSV) |
| 7 | Failure: Gerät mitten im Lauf getrennt | live | sauberer „getrennt"-Zustand, kein Crash, Reconnect möglich | [ ] live |
| 8 | Failure: kaputte/Teil-JSON-Zeile | live/auto | verworfen + gezählt, kein Absturz | [x] 2026-06-25 („{kaputt" verworfen=1; „CK1…" ignoriert; kein Crash) |
| 9 | `gas_raw=null` / fehlende Felder | auto | „—" statt 0 dargestellt | [x] 2026-06-25 (null → „—") |
| 10| Leerzustand bei kleinem n | auto | abgeleitete Größen sagen „zu wenig Daten (n=…)" statt Scheinzahl | [x] 2026-06-25 (Footer n<30 → Meldung; bei n=3 aktiv) |

## Abhängigkeiten
- **COM8 muss frei sein:** Browser-Web-Serial und mein `arduino-cli`-Serial können den Port **nicht gleichzeitig** halten.
- **Chrome/Edge** (Web Serial; kein Firefox/Safari).
- Live-Verifikation ist eine **Browser-Aktion des Nutzers** (Web Serial ist user-gesten-gated) — ich kann sie von hier nicht auslösen. Demo-Modus für den HW-freien Check.

## Stufe 1b (später, NICHT in 1a)
Pearson-Korrelationsmatrix + Scatter/r² + **Pflicht-Qualifizierung** (n, Zeitfenster, Störquellen-Warnung:
Eigenerwärmung koppelt `air_temp_c/humidity_pct/soil_temp_c/gas_raw` → mögliche Scheinkorrelation).

## Echtzeit-Streams (Erweiterung, 2026-06-25)
Firmware streamt zusätzlich über Serial: `N:<rms>` (~5 Hz Lärm) und `L:<lux>,<uv>,<r>,<g>,<b>` (~2 Hz Licht).
Regen bleibt **1×/min** (nur im JSON). Streams gehen **nicht** in `records` und **nicht** in den POST.

| # | Test | Art | Erfolgssignal | Status |
|---|------|-----|---------------|--------|
| 11 | `N:`-Stream → Live-Lärm-Meter | auto | Balken/Peak/Sparkline live; nicht in records | [x] 2026-06-25 (Preview) |
| 12 | `L:`-Stream → Live-Licht (Lux-Balken, UV, Farbswatch) | auto | live; nicht in records | [x] 2026-06-25 (Preview) |
| 13 | Regen via 1-min-Record | auto | rain-Tile aus JSON, nicht aus Stream | [x] 2026-06-25 (rain:true → „JA ☔") |
| 14 | Firmware: `N:` ~5 Hz + `L:` ~2 Hz am Gerät, JSON 1-min intakt | live | Streams am COM8, JSON weiterhin 1/min, kein Crash | [x] 2026-06-25 (N:36/L:25 in 16 s, sauberer Boot) |
| 15 | WLAN-Live über **lokalen Geräte-SSE-Server (Pfad B)** → Browser `EventSource` → `handleLine`. **Ungesichert:** HTTP, kein HMAC/TLS, `CORS *`. **≠ signierter n8n-Pfad A (P7).** | live | Panel zeigt `N:`/`L:`/JSON **über WLAN direkt vom Gerät** | [x] 2026-06-25 LIVE: Browser→`192.168.1.245`, 26 Zeilen/5 s, `opened:true`, Meter aktualisiert. Beweist **„lokaler Stream läuft"**, NICHT „n8n-Pipeline läuft". |

**Pfad A (signierter HTTPS-POST → n8n, P7) bleibt OFFEN:** WLAN-Creds provisioniert (NTP synchron), aber n8n-URL/HMAC + echte CA (`LE_ROOT_CA` noch Platzhalter) nicht verifiziert. Pfad B sagt dazu nichts.

## Abnahme Web-Serial-Pfad (2026-06-25, Kriterien vom Nutzer)
Gerät aktuell an **COM9** (re-enumeriert von COM8 nach Replug). Web-Serial = Browser-Geste des Nutzers; COM darf dabei von nichts anderem gehalten werden.

| # | Kriterium | Erfolgssignal | wer belegt | Status |
|---|-----------|---------------|-----------|--------|
| A1 | Echte JSON-Zeile vom ESP32 | wohlgeformte 18-Feld-JSON am COM-Port, Real-Timestamp | Claude | belegt 2026-06-25 COM11: 2 reale Zeilen, 18 Felder, Timestamp `17:37:51Z` (NTP synchron) — Urteil Nutzer |
| A2 | Erfolgreicher Web-Serial-Empfang | Edge verbindet CH340/COM11, Zeilen im Panel | Nutzer (Edge) | offen — dein Klick (COM11 frei). **Präzise:** der Klick beweist **Funktion** (Port öffnet, Zeilen kommen, Records zählen, Felder rendern), **nicht Dauerstabilität über Stunden** → das ist #7 (Trennen/Reconnect) + Powerbank-Lauf, separat |
| A3 | Record-Zähler steigt 1×/min | +1 Record/min | Claude/Nutzer | Panel-Records +1/min **belegt**: 3 Records exakt 60 s (`17:44:01`→`17:45:01`→`17:46:01`). `seq=0` in allen — **korrekt, kein Bug:** `seq` zählt nur bei erfolgreichem n8n-POST (Replay-Schutz P7); Pfad A aus → `seq` MUSS 0 sein (`seq>0` ohne POST *wäre* der Fehler). Lesart A3 = **Panel-Records** (erfüllt) ODER **Geräte-`seq`** (braucht Pfad A) → Nutzer entscheidet am Testplan |
| A4 | Korrekte Feldzuordnung, keine Validierungsfehler | alle Felder, 0 verworfen, kein Feld fälschlich „—" | Claude | belegt 2026-06-25: echte Zeile → Parser: 1 Record, alle Felder gemappt, `gas_raw=null` erhalten, 0 verworfen — Urteil Nutzer |
