# Abnahmeprotokoll — WiFi-Drop-Test (UmweltMonitor Basismodul)

> Freigegeben nach WiFi-Umbau v2 (nicht-blockierender Reconnect, überlaufsicher).
> Beweist oder widerlegt: Das Gerät erholt sich nach WLAN-Abriss selbstständig.
> Statische Code-Prüfung: 10/10 bestanden. Jetzt der Laufzeit-Beweis.

---

## Voraussetzungen

- Firmware `umweltmonitor_basis.ino` (WiFi v2) kompiliert und geflasht
- Panel `umweltmonitor_panel.html` im Browser geöffnet, verbunden via `http://umweltmonitor.local`
- Serielle Konsole offen (115200 Baud) zum Mitschneiden der Event-Logs
- Gerät im WLAN, IP notiert (z. B. `192.168.1.245`)
- Kein n8n-POST nötig — dieser Test prüft nur Pfad B (SSE-Stream)

---

## Test A — Einfacher Drop (gleiche IP wahrscheinlich)

**Setup:** Gerät läuft, SSE-Stream sichtbar im Panel (N:- und L:-Zeilen).

**Aktion:**
1. Router vom Strom trennen (oder WLAN am Router deaktivieren)
2. 30 Sekunden warten
3. Router wieder einschalten und warten, bis er hochgefahren ist

**Erwartung:**
- Serielle Konsole zeigt `WLAN-WEG (Grund X): <text>` (Event-Handler)
- Nach Router-Hochlauf: `WLAN-Wiederverbindung...` (Health-Check)
- Dann: `WLAN-IP: <ip>` mit RSSI (Event-Handler `STA_GOT_IP`)
- Panel: SSE-Stream setzt von selbst fort — **kein** Reload, **kein** manueller Eingriff
- Zeit vom Router-Up bis erstem neuen N:/L:-Datenpunkt notieren

**Bestanden wenn:** Stream erscheint im Panel ohne Nutzer-Aktion.

**Nicht bestanden wenn:** Panel zeigt dauerhaft „Verbindung verloren" oder erfordert Reload.

| Messung | Wert |
|---|---|
| Zeit Router-Aus → `WLAN-WEG` geloggt | ___ s |
| Zeit Router-Up → `WLAN-IP` geloggt | ___ s |
| Zeit Router-Up → erster N:/L:-Datenpunkt im Panel | ___ s |
| IP vor Drop | _________ |
| IP nach Reconnect | _________ |
| Panel-Reload nötig? | ja / nein |
| **Test bestanden?** | ✅ / ❌ |

---

## Test B — Drop mit IP-Wechsel (Härtefall)

**Setup:** Gerät läuft, SSE-Stream sichtbar, IP notiert.

**Aktion:**
1. Router vom Strom trennen
2. **Wichtig:** DHCP-Lease im Router ablaufen lassen (entweder 5+ Minuten warten, oder Lease auf dem Router manuell löschen / Gerät aus der DHCP-Tabelle entfernen)
3. Router wieder einschalten

**Erwartung (zusätzlich zu Test A):**
- Gerät bekommt eine **andere** IP als vorher (da Lease abgelaufen)
- Serielle Konsole: `WLAN-IP: <neue_ip>` (andere als vorher)
- `mDNS reaktiviert` erscheint im Log
- Panel unter `http://umweltmonitor.local` zeigt weiterhin den Stream (mDNS hat neue IP aufgelöst)
- Falls Panel auf feste IP zeigt: **erwartet fehlgeschlagen** — das ist der Schwachpunkt, den Task #4 adressiert. Panel muss auf mDNS umgestellt werden.

**Bestanden wenn:** Stream unter `umweltmonitor.local` sichtbar, neue IP geloggt, mDNS-Log vorhanden.

**Nicht bestanden wenn:** Stream nur unter fester IP erreichbar, oder mDNS löst nicht auf.

| Messung | Wert |
|---|---|
| IP vor Drop | _________ |
| IP nach Reconnect | _________ |
| IP gewechselt? | ja / nein |
| `mDNS reaktiviert` im Log? | ja / nein |
| `umweltmonitor.local` erreichbar? | ja / nein |
| Panel-Reload nötig? | ja / nein |
| **Test bestanden?** | ✅ / ❌ |

---

## Test C — Kaltstart ohne WLAN (Router aus beim Boot)

**Setup:** Router aus. Gerät vom Strom trennen (USB raus/rein), serielle Konsole offen.

**Aktion:**
1. Gerät einschalten (USB einstecken) während Router aus ist
2. Serielles Log beobachten
3. Nach 30 s Router einschalten

**Erwartung:**
- Boot-Log: `WLAN-Verbindung zu <ssid>.............. WARN: Erstverbindung fehlgeschlagen — retry im Loop`
- Gerät läuft weiter (kein Crash, kein Boot-Loop)
- Sensorik initialisiert (CK1/CK2/CK3 erscheinen)
- Nach Router-Up: Health-Check greift → `WLAN-Wiederverbindung...` → `WLAN-IP: <ip>`
- SSE-Stream startet

**Bestanden wenn:** Gerät bootet sauber ohne WLAN, verbindet nach Router-Up, Stream startet.

| Messung | Wert |
|---|---|
| `WARN: Erstverbindung fehlgeschlagen` geloggt? | ja / nein |
| CK1/CK2/CK3 nach Boot? | ja / nein |
| Zeit Router-Up → `WLAN-IP` | ___ s |
| Stream im Panel? | ja / nein |
| **Test bestanden?** | ✅ / ❌ |

---

## Zusammenfassung

| Test | Szenario | Bestanden |
|---|---|---|
| A | Router aus/an, gleiche IP | ⬜ |
| B | Router aus/an, neue IP (mDNS) | ⬜ |
| C | Kaltstart ohne WLAN | ⬜ |

Alle drei bestanden → WiFi-Umbau ist **bewiesen**, nicht nur plausibel.
