# Firmware-Prompt — Umweltmonitor-Basismodul (Kalletal-Zeltplatz)

> Maßgebliche, finalisierte Fassung (2026-06-24). Ersetzt frühere C3-/Sternnetz-Entwürfe.
> Self-contained für einen frischen Chat.

---

**Projekt: ESP32-Firmware für das Umweltmonitor-Basismodul (Kalletal-Zeltplatz)**

Ich baue die Firmware für das **Basismodul** eines fest installierten Umweltmonitors an meinem
Zeltplatz. Dieser Chat hat **keinen Kontext** aus früheren Chats — alles Nötige steht hier.

**Topologie (bestätigt):**
- **Ein Basismodul-ESP32** ist der einzige aktive Knoten und Uplink.
- **Himmelssonde** ist in das Basismodul **integriert** (steht baulich etwas nach oben weg).
- **Bodensonde** ist **per Kabel** angebunden (kein Funk).
- **Kein Funk zwischen Knoten** — frühere Sternnetz-/ESP-NOW-/LoRa-Überlegungen sind
  gegenstandslos. Alle Sensoren hängen per GPIO/ADC/I²C/UART am Basismodul.

**Datenweg (bestätigt):**
- Basismodul misst periodisch → **HTTPS-POST an n8n** (`https://<N8N_HOST>`)
  → Google Sheet „<SHEET_DOC>" (analog zur bestehenden Wetterstation „Erder").
- Ausgabe als **strukturiertes JSON**: klare Feldnamen + Einheiten + ISO-Zeitstempel +
  `device_id` + `schema_version`. Schema ist **additiv** (neue Sensoren = neue Felder,
  nie umbenannte/entfernte).

**Sicherheit (bestätigt — Schreib-Pfad):**
- **HTTPS mit Zertifikatsprüfung** (kein `setInsecure()`): Server-Zert/CA bzw. Fingerprint
  im Gerät hinterlegt und geprüft. Klartext-HTTP-Fallback hart deaktiviert.
- **HMAC-SHA256** über den Payload mit gemeinsamem Geheimnis (ESP32-Hardware-SHA) →
  Authentizität + Integrität. **Replay-Schutz** über mitsignierten monotonen Zähler/Timestamp.
- **Secrets nicht im Code:** WLAN-Passwort, Webhook-URL, HMAC-Schlüssel in **NVS**, nicht
  hardcodiert, nicht im Git. Provisioning einmalig (seriell oder Captive-Portal).
- **Watchdog + Brownout-Detection** aktiv (fest installiert, Verfügbarkeit zählt).

**Bewusst NICHT in dieser Firmware (kein Theater):**
- Keine Funk-Anbindung, kein Funk-Krypto.
- Kein OTA im Testaufbau (Updates per USB vor Ort) — kleinste Angriffsfläche.
- Kein Secure Boot / keine Flash Encryption (eFuse-Einweg, Brick-Risiko, unverhältnismäßig
  für einen Sensor, dessen Secret nur *ein* Schreibstream berechtigt).
- Keine Lese-API in der Firmware — die kommt separat als FastAPI auf dem VPS.

**ZUERST FESTLEGEN (offen — am Chat-Anfang klären, NICHT raten):**
1. **ESP32-Variante & freie Pins.** Achtung: **ADC2 ist bei aktivem WLAN gesperrt** →
   alle Analogkanäle auf **ADC1** legen.
2. **Finale Sensorliste + Modelle.** Sauber benennen, was real gemessen wird:
   - CO₂: echtes NDIR (`co2_ppm`, z. B. SCD40/41 oder MH-Z19C) vs. geschätztes eCO₂
     (`eco2_ppm`, z. B. SGP40) — **nicht** vermischen.
   - Lärm: nur **relativer Pegel** (`noise_rel`/`noise_dbfs`), **kein** `db_spl` ohne Referenz.
   - UV/Licht: aktuelle Teile (z. B. LTR390 / GUVA-S12SD; BH1750/TSL2591 für Lux), nicht das
     abgekündigte VEML6075.
   - Regen: Wippe = digitaler Impuls (Interrupt-Pin), Bodenfeuchte kapazitiv = analog (ADC1).
3. **Sende-Intervall** (Vorgabe der Station: ~10 min — bestätigen oder ändern).
4. **Stromversorgung** (fest = Netz, oder Akku/Solar → dann Brownout/Watchdog kritisch).
5. **TLS-Pinning-Art:** CA (Let's Encrypt) oder Fingerprint — hängt davon ab, ob Caddy
   auf `…sslip.io` ein gültiges LE-Zert oder ein selbstsigniertes liefert.

**Vorgehen (bitte einhalten):**
1. **Vor dem Code einen Testplan** (`TESTPLAN.md`) anlegen und fortschreiben.
2. Erst **kurzer Plan** (Pins, ADC1-Kanäle, Sensor-Bibliotheken, Abtastraten, JSON-Format,
   HMAC-/NVS-Aufbau), dann Code.
3. **Bewiesen vs. vermutet strikt trennen.** Festgelegte Werte (Pins, Intervalle, Secrets)
   nicht eigenmächtig ändern.
4. **Hardware-Realität zuerst klären** (siehe „ZUERST FESTLEGEN").
5. **Projektdateien** unter `…\02_PROJECTS\WetterStudioTeam\UmweltMonitor\firmware\…` ablegen.
6. Bei automatisierten Schritten: **kein Retry-Loop bei Bypass, maximal 3 Versuche.**

**Fertig, wenn (= verifiziert, nicht behauptet):**
1. Alle gewählten Sensoren werden plausibel auf ADC1/I²C/UART eingelesen (Live-Wert je Kanal).
2. Firmware gibt strukturiertes JSON (Feldnamen + Einheiten + ISO-Zeit + `device_id` +
   `schema_version`) aus.
3. POST geht **nur** über HTTPS mit geprüftem Zertifikat; falsches Zert → Verbindung
   schlägt fehl (Negativtest).
4. Payload ist HMAC-signiert; manipulierter Payload → von n8n verworfen (Negativtest,
   sofern n8n-Seite verfügbar — sonst lokal Signatur-Selbsttest).
5. Alter signierter Payload erneut gesendet → durch Zähler/Timestamp abgelehnt.
6. Secrets liegen in NVS, nicht im Quellcode/Git.
7. `TESTPLAN.md` dokumentiert die Live-Verifikation je Punkt.
