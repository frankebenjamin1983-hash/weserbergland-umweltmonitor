# Sicherheitsarchitektur — Umweltmonitor

> ⚠️ **SUPERSEDED (2026-06-24, später am selben Tag):** Topologie auf **Einzel-Basismodul ohne
> Funk** geändert — Bodensonde **per Kabel**, Himmelssonde **integriert**, kein Sternnetz/
> ESP-NOW/LoRa. Das gesamte **Funk-Segment unten ist gegenstandslos** und bleibt nur als
> Audit-Trail stehen. **Weiter gültig:** HTTPS + Zert-Prüfung, HMAC + Replay, Secrets in NVS,
> Watchdog/Brownout — jetzt auf der Strecke **Basismodul → n8n**. Maßgeblich ist
> [`prompts/firmware-prompt.md`](prompts/firmware-prompt.md).

## (historisch) Sternnetz mit Funk-Segment

## Kontext / Architektur-Änderung (2026-06-24)
Zwischen den Knoten kommt ein **Funk-Segment** dazu → aus einem Gerät wird ein **Sternnetz**:
mehrere Sensor-ESPs (Bodensonde, Himmelssonde, …) → **ein Gateway-ESP** → n8n. Das Funk-Segment
ist ab jetzt die **exponierteste Stelle**: Funk ist Broadcast — jeder in Reichweite kann mithören
und (schlimmer) Frames einspeisen.

```
[Bodensonde-ESP]  ┐
[Himmelssonde-ESP]├──Funk──►  [Gateway-ESP] ──HTTPS+HMAC──► n8n ──► Sheets/Warnmodul
[weitere Sonden]  ┘            (einziger Uplink)
```

## Bestätigt — gilt unabhängig vom Funkprotokoll
- **Kompartmentierung (Kernprinzip):** Nur der **Gateway** kennt das n8n-Secret und hat den
  WLAN-/Starlink-Uplink. Sondenknoten kennen **nur ihren Funk-Schlüssel**, nie n8n-Credentials.
  → Wird eine Sonde physisch entwendet, leakt nur ihre Funk-Identität, **nicht** der Pipeline-Zugang.
- **Gateway → n8n unverändert:** HTTPS mit Zertifikatsprüfung + HMAC-signierter Payload +
  Range-Check + Replay-Schutz in n8n. Gateway aggregiert die **verifizierten** Sondenwerte und
  sendet **einen** signierten Sammel-POST.
- **Zwei Schutzaufgaben im Funk-Segment:**
  1. **Authentizität jedes Frames** — IDs sind fälschbar; der Gateway glaubt einem Frame nicht
     wegen der Geräte-ID. Jeder Frame wird **signiert / authentisiert-verschlüsselt**; ungültige
     werden verworfen. (Funk-Pendant zum HMAC-Webhook.)
  2. **Replay-Schutz** — **monotoner Zähler pro Sonde**; Gateway merkt sich den letzten Stand und
     lehnt alte/gleiche Frames ab (sonst friert ein Angreifer einen alten Messwert ein).
- **Vertraulichkeit der Werte = niedrigprioritär** (Lux/Bodenfeuchte ist kein Geheimnis) —
  **Authentizität + Replay** sind das, was zählt.
- **Key-Provisioning:** einmalig **seriell pro Sonde in NVS** schreiben (kein OTA-Pairing im
  Testaufbau → kleinste Angriffsfläche). Optional Pairing-Knopf mit zeitlich begrenztem Fenster
  für Feldtausch.

## OFFEN — muss entschieden werden, nicht geraten
### A) Funkprotokoll — ENTSCHIEDEN: ESP-NOW (2026-06-24)
**Eingaben:** Distanz ≤ ~50 m & freie Sicht · Versorgung Kabel/Netz · Intervall ~1 min (Himmelssonde).
**→ ESP-NOW:** Reichweite reicht klar; 1-min-Intervall unkritisch (kein Duty-Cycle wie bei LoRa);
eingebautes AES; Kompartmentierung bleibt erhalten (kein n8n-Credential auf den Sonden). LoRa
unnötig (keine Großdistanz/Boden-Durchdringung gefragt); WLAN-STA verworfen (bricht Kompartmentierung).

**ESP-NOW-Umsetzungs-Caveats (umzusetzen + zu testen):**
- Gateway fährt WLAN-Uplink **und** ESP-NOW gleichzeitig → **alle Peers auf demselben WLAN-Kanal**
  wie der Gateway-Uplink. Kanal fest setzen.
- **Verschlüsselte Peers ~10** → reicht für Boden-/Himmelssonde (+ Reserve); bei mehr Knoten einplanen.
- **Replay trotz AES selbst lösen:** monotoner Zähler im Payload, Gateway verwirft ≤ letzter Stand.

Vergleich (warum nicht die anderen):
| Option | Pro | Contra |
|---|---|---|
| **ESP-NOW** (2,4 GHz) | kein Router nötig, sparsam, **AES (PMK/LMK) eingebaut** → Auth+Vertraulichkeit teils geschenkt, geringe Latenz | Reichweite real ~zig–150 m, durch Boden/Hindernisse weniger; Gateway muss WLAN+ESP-NOW auf **demselben Kanal** fahren; verschlüsselte Peers begrenzt (~10); **Replay trotz AES selbst** per Zähler |
| **LoRa** (868 MHz EU) | sehr hohe Reichweite, durchdringt Vegetation/Boden besser, Sonden weit absetzbar | **keine PHY-Verschlüsselung** → AES+Auth selbst; **Duty-Cycle ~1 %** (passt zu 10-min, sprengt Hochfrequenz-Sampling); niedrige Datenrate |
| **WLAN-STA pro Sonde** | einfachster Datenweg, nutzt vorhandene HMAC+TLS-Strecke, kein Funk-Krypto | **kippt die Kompartmentierung** (jede Sonde hält n8n-Creds = Einfallstor), höherer Verbrauch → schwächster Sicherheitsentwurf; nur als Übergangs-Testaufbau |

**Krypto — gewählt: ESP-NOW (LoRa/WLAN nur zum Vergleich):**
- **ESP-NOW:** AES (PMK/LMK) aktiv + mitgesendeter **Zähler** für Replay; Authentizität aus dem Pairing (nur LMK-Peers akzeptiert).
- **LoRa:** **AES-CCM/GCM (AEAD)** per-Knoten-Schlüssel, **Nonce = Zähler** → Auth + Replay + optional Vertraulichkeit in einem Schritt (Libs auf ESP32 vorhanden).
- **WLAN-STA:** kein Funk-Krypto, aber Kompartmentierung verloren (s. o.).

### B) Topologie-Konflikt mit der „Konsolidierung"
Dieses Sternnetz **widerspricht** der früheren Entscheidung „ein Board (konsolidiert)" und dem
gelieferten `firmware/umweltmonitor-komplettschema.svg` (= ein Board mit allen Sensoren).
→ Zu klären: Welche Sensoren sitzen auf **Bodensonde** vs. **Himmelssonde** vs. **Gateway**?
Das Komplettschema gilt dann nur noch für einen Knoten/den Gateway.

## Verifikation (fertig = verifiziert, um Funk erweitert)
- Gefälschter Funk-Frame (falsche Signatur) → Gateway **verwirft + loggt**.
- Mitgeschnittener gültiger Frame erneut gesendet → **abgelehnt** (Zähler).
- Sonde mit unbekanntem Schlüssel → **kein Peering, ignoriert**.
- **Reichweitentest je Sonde** am echten Standort (Boden tief / Himmel hoch) → RSSI/Verlustrate dokumentiert.
- **Kompartmentierung geprüft:** Sondenknoten enthält (Code/NVS-Review) **keine** n8n-Credentials.
