# Logik-Flussdiagramm — Umweltmonitor-Basismodul (Firmware v1)

Rendert in jedem Mermaid-fähigen Viewer (GitHub, VS Code + Mermaid-Extension, https://mermaid.live).
Stand 2026-06-25: inkl. **Echtzeit-Streams** (`N:`/`L:`) und **WLAN-Live-Stream (SSE)** — beides neben dem signierten 60-s-POST.

```mermaid
flowchart TD
  A(["Power-On / Reset"]) --> B["setup: ADC, GPIO, I2C Bus0 21/22 + Bus1 18/19"]
  B --> C["Sensoren init + Presence-Gate: AHT20, BMP280 @0x77, TCS34725, TSL2591, TMP102, FRAM @0x50"]
  C --> D["MiCS-EN HIGH, Watchdog 30s, NVS-Secrets laden, seq aus FRAM (Fallback NVS)"]
  D --> E["WLAN + NTP"]
  E --> F["setSleep(false) + AutoReconnect, SSE-Server :80 + mDNS"]
  F --> G(["loop()"])
  G --> H["sseAccept + Timer pruefen"]
  H --> T1{"200 ms: Schall"}
  H --> T2{"500 ms: Licht"}
  H --> T3{"60 s: Vollzyklus"}
  T1 -->|ja| SN["Schall-RMS, Zeile 'N:'"]
  T2 -->|ja| SL["Licht Lux/UV/RGB, Zeile 'L:'"]
  T3 -->|ja| J["alle Sensoren lesen"]
  J --> JG{"MiCS warm > 60s?"}
  JG -->|nein| M["JSON: device_id, seq, timestamp, Werte"]
  JG -->|ja| M
  SN --> OUT["USB-Serial + WLAN-SSE (Pfad B, ungesichert)"]
  SL --> OUT
  M --> OUT
  OUT --> PANEL(["Panel: Web-Serial ODER WLAN-EventSource"])
  M --> P{"NVS-Secrets da?"}
  P -->|ja| SG["HMAC signieren + HTTPS-POST n8n CA-geprueft (Pfad A)"]
  SG --> R{"HTTP 2xx?"}
  R -->|ja| SQ["seq++ in NVS (Replay-Schutz)"]
  SQ --> SHEET(["n8n -> Google Sheet"])
  R -->|nein| WD["Watchdog reset"]
  P -->|nein| WD
  SQ --> WD
  OUT --> WD
  T1 -->|nein| WD
  T2 -->|nein| WD
  T3 -->|nein| WD
  WD --> G
```

## Zwei getrennte Datenwege
- **Pfad A — signierter HTTPS-POST → n8n (1×/min):** HMAC-SHA256 über den exakten JSON-Body, CA-geprüftes HTTPS (kein `setInsecure()`). `seq` zählt **nur bei HTTP 2xx** hoch (monoton, NVS-persistent → Replay-Schutz). Braucht provisionierte `url`+`hmac`.
- **Pfad B — lokaler Live-Stream (Echtzeit):** `N:` (~5 Hz Schall) und `L:` (~2 Hz Licht) plus die 1-min-JSON gehen über **USB-Serial UND WLAN-SSE** (Port 80, `umweltmonitor.local`, CORS offen) direkt ans Panel. **Ungesichert** — nur fürs lokale WLAN gedacht, NICHT der signierte Pfad A.

## Weitere Kernpunkte
- **MiCS-Warmup:** erste ~60 s `gas_raw=null`, bis der MOS-Sensor warm ist.
- **WLAN-Stabilität:** `setSleep(false)` (kein Modem-Sleep) + Auto-Reconnect → weniger Stream-Drops.
- **Presence-Gating:** jeder I²C-Sensor wird vor `begin()` per ACK geprüft → kein Crash bei fehlendem Gerät.
- **Kein Deep-Sleep** (Netzbetrieb); durchgehender Takt per `millis()`. USB-Serial schläft nie.
