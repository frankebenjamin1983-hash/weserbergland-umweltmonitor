# Lastenheft — Gehäuse & Bodensonde (Umweltmonitor Kalletal)

Kennzeichnung: **M** = Muss, **S** = Soll, **K** = Kann. Stand 2026-06-25.

## 1. Zweck & Rahmen
Anforderungen an (A) das Gehäuse des fest installierten ESP32-Umweltmonitors und (B) die abgesetzte,
gekabelte Bodensonde. Ziel: zuverlässiger Dauer-Außenbetrieb (Weserbergland), wartungsarm, messwert-treu.
- Umgebung: dauerhaft außen, Wesernähe; feucht, Winterfrost, direkte Sommer-Sonne.
- Versorgung: 5-V-Netzteil → VIN; Netzbetrieb, kein Akku/Solar.
- Elektronik: ESP32-D0WD NodeMCU 30-Pin + Breakout-Sensoren (siehe `../firmware/anschlussliste.md`).

## 2. Gehäuse
### 2.1 Schutz / Material
- M: Elektronikfach ≥ IP54, Ziel IP65; Deckel gedichtet.
- M: Druckausgleich gegen Kondens (PTFE-Membran); Sensorfach **nicht** luftdicht.
- M: UV-/witterungsfestes Material (ASA, PETG-CF, PC) — **kein PLA**.

### 2.2 Thermik (Voraussetzung Messtreue)
- M: Luftsensor (AHT20/BMP280) thermisch vom ESP32 entkoppelt (eigenes belüftetes Teilvolumen/Abstand); Ziel Eigenerwärmungs-Offset < 1 K.
- M: weißer/reflektierender Strahlungsschutz (Lamellen/Stevenson-Art) für die Luftmessung.
- S: ESP32-Abwärme nach oben/weg vom Luftsensor führen.

### 2.3 Belüftung
- M: belüftetes Sensorvolumen → gemessene Luft = Umgebungsluft (Bedingung für Taupunkt-Gültigkeit), kein stehendes Luftpolster.
- M: kein Regeneintrag in den belüfteten Bereich (Lamellen/Labyrinth); Insektengitter.

### 2.4 Optische Sensoren
- M: GUVA (UV), TSL2591 (Lux), TCS34725 (Farbe) mit **freier, unverschatteter Himmelssicht nach oben**; klares/UV-durchlässiges Fenster oder offene, regengeschützte Öffnung.
- S: definierte Ausrichtung (horizontal nach oben), dokumentiert.

### 2.5 Gas / Schall / Regen
- S: MiCS (Gas) + MAX9814 (Schall) an freier Luft, weg von Eigenabwärme; Mikrofon mit Wind-/Spritzschutz.
- M: Raindrop-Platte regenexponiert und ablaufend (kein Stauwasser).

### 2.6 Durchführung / Montage / Service
- M: abgedichtete Kabelverschraubungen für Bodensonde + 5-V-Zuleitung.
- M: Mastmontage, windstabil; Luftmessung ~1,25–2 m über (Gras-)Boden, Abstand zu Wänden/Wärmeinseln.
- M: USB-Port ohne Demontage erreichbar (Flashen vor Ort) — abgedichtete Serviceklappe.
- S: werkzeugarm öffenbar; Sensoren als Steckmodule tauschbar.

### 2.7 Korrosion
- M: bestromte Nässe-Sonden (HW-038, Raindrop) entweder (a) per MOSFET gegated oder (b) dokumentiertes Verschleißteil mit Tauschintervall — **Gehäuse-Abdichtung schützt diese Kontakte NICHT**.
- S: Platine mit Conformal Coating.

## 3. Bodensonde (abgesetzt, gekabelt)
- M: enthält HW-390 (kapazitive Bodenfeuchte) + TMP102 (Bodentemp, I²C); abgesetzt vom Hauptgehäuse.
- M: TMP102 + Verdrahtung **wasserdicht vergossen** (Epoxid/Potting); HW-390 gemäß Herstellervorgabe bis Messbereich vergossen.
- M: I²C (TMP102) + Analog (HW-390) über das Kabel; **Kabel so kurz wie möglich** (I²C-Kapazität) — Ziel ≤ ~1–2 m, sonst niedrigere I²C-Taktrate / Bus-Puffer prüfen.
- M: stabile 3,3-V-Versorgung (HW-390 ratiometrisch), gemeinsame Masse; S: feuchtefestes/geschirmtes Kabel + Zugentlastung.
- M: definierte, reproduzierbare Einbautiefe (z. B. 10 cm), dokumentiert; S: zerstörungsfrei entnehmbar.
- M: HW-390 end-to-end kalibriert (trocken/nass → VWC), Rohwert bleibt verfügbar; S: TMP102-Offset gegen Referenz.

## 4. Abnahmekriterien
- Dichtigkeit: Sprühtest → Elektronikfach trocken.
- Thermik: Luftsensor-Offset gegen Referenz < 1 K bei Sonne/Betrieb.
- Belüftung: Taupunkt innen ≈ außen (Innen-Luft folgt Außen-Luft).
- Bodensonde: nach Wässerungstest dicht, plausible Feuchte/Temperatur.
- Service: Flashen ohne Gehäuseöffnung möglich.
- Langzeit: nach Testzeitraum kein Wasser-/Korrosionsbefund (Sichtprüfung).
