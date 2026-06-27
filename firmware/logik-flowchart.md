# Logik-Flussdiagramm — Umweltmonitor-Basismodul (Firmware v1)

Stand 2026-06-27. Rendert in jedem Mermaid-fähigen Viewer (GitHub, VS Code + Mermaid-Extension, https://mermaid.live).

```mermaid
flowchart TD
  A(["Power-On / Reset"]) --> RR["Reset-Grund loggen\nesp_reset_reason()\nPOWERON / BROWNOUT / PANIC / WDT"]
  RR --> B["setup: ADC 12-bit, GPIO, WDT 30s\nI²C-Bus0 GPIO21/22 + Bus1 GPIO18/19"]
  B --> C["Sensoren Presence-Gate + init\nAHT20·BMP280·TCS34725·TSL2591·TMP102·FRAM@0x50"]
  C --> TFT["TFT init (SPI-HSPI GPIO13/14/25/16/4/17)\nBL HIGH, fillScreen schwarz"]
  TFT --> D["NVS-Secrets laden\nseq aus FRAM (Magic 0xDEADBEEF) — Fallback NVS"]
  D --> E["WLAN + NTP\nAutoReconnect, setSleep(false), Event-Handler"]
  E --> F["SSE-Server :80 + mDNS umweltmonitor.local"]
  F --> G(["loop()"])

  G --> WDT["esp_task_wdt_reset()"]
  WDT --> WW{"WLAN-Check\nalle 30s"}
  WW -->|verbunden| G2["wifiFailCount = 0\nmDNS sicherstellen"]
  WW -->|getrennt| WF["wifiFailCount++\nGrund loggen"]
  WF -->|"≥ 10 (5 min)"| RS["ESP.restart()"]
  WF -->|"== 1"| RC["WiFi.begin() erneut"]
  G2 --> H
  RC --> H

  H["sseAccept()"] --> T1{"200 ms: Schall"}
  H --> T2{"500 ms: Licht+Farbe"}
  H --> T3{"60 s: Vollzyklus"}

  T1 -->|ja| SN["Schall-RMS → N:<rms>"]
  T2 -->|ja| SL["TSL2591 + TCS34725\ndV = ΔR+ΔG+ΔB\n→ L:<lux>,<uv>,<r>,<g>,<b>,<dV>"]
  T3 -->|ja| J["alle Sensoren lesen\nJSON bauen (18 Felder)"]

  SN --> OUT["USB-Serial + WLAN-SSE (Pfad B)"]
  SL --> OUT
  J --> OUT

  OUT --> PANEL(["Panel: Web-Serial / WLAN-EventSource"])

  J --> P{"NVS-Secrets\nvorhanden?"}
  P -->|ja| SG["HMAC-SHA256 signieren\nHTTPS-POST n8n (CA-geprüft, ISRG Root X1)"]
  SG --> R{"HTTP 2xx?"}
  R -->|ja| SQ["seq++\nsaveSeq: FRAM (primary) / NVS (fallback)"]
  SQ --> TFT2["tftShow: Temp·Feuchte·TP·Druck\nLux·UV·dV | FRAM·WLAN·Uptime"]
  R -->|nein| WDT
  P -->|nein| WDT
  SQ --> SHEET(["n8n → Google Sheet"])
  TFT2 --> WDT
```

## Datenwege

- **Pfad A — signierter HTTPS-POST → n8n (1×/min):** HMAC-SHA256, CA-geprüftes HTTPS. `seq` steigt nur bei HTTP 2xx (Replay-Schutz). Braucht NVS-Secrets (`url` + `hmac`). *Noch nicht aktiv — Provisioning ausstehend.*
- **Pfad B — lokaler Live-Stream (Echtzeit):** `N:` (~5 Hz Schall), `L:` (~2 Hz Licht+dV) und 1-min-JSON über USB-Serial **und** WLAN-SSE (Port 80, CORS offen). Ungesichert — nur lokales WLAN.

## Kernpunkte

- **Reset-Grund:** `esp_reset_reason()` bei jedem Boot → POWERON/BROWNOUT/PANIC/TASK_WDT/SW_RESET. Diagnose bei Ausfällen.
- **WLAN-Watchdog:** nach 5 min ohne WLAN → `ESP.restart()`. Disconnect-Grund (Code + Text) geloggt.
- **FRAM-Persistenz:** seq-Zähler in MB85RC256V (0x50, Magic 0xDEADBEEF). NVS nur wenn kein FRAM.
- **TFT-Display:** 1×/min aktualisiert nach Messzyklus. Lux/UV/dV, Status-Zeile (FRAM/WLAN/Uptime).
- **Farbvarianz dV:** Summe |ΔR|+|ΔG|+|ΔB| zwischen TCS34725-Messungen (500ms). Lichtdynamik-Indikator.
- **Presence-Gating:** I²C-ACK vor jedem `dev.begin()` → kein Crash bei fehlendem Gerät.
- **MiCS-Warmup:** erste ~3 min `gas_raw=null`.
- **Kein Deep-Sleep** (Netzbetrieb), durchgehender Takt per `millis()`.
