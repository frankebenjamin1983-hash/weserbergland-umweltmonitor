> **Projekt: Gehäusebauform, Sensoranordnung und Standort für den Umweltmonitor (Kalletal-Erder)**
>
> Ich plane das **physische Design und die Aufstellung** eines ESP32-Umweltmonitors für den
> Außeneinsatz. Dieser Chat hat **keinen Kontext** aus früheren Chats. Es geht **nicht** um
> Firmware/Datenpipeline und **nicht** um EM-Sonden/Elektrosmog (separate Teilprojekte).
> Hier geht es ausschließlich um: **welche Gehäusebauform, welche Anordnung der Sensoren
> zueinander, und welcher Standort/welche Position** die Messwerte möglichst unverfälscht macht.
>
> **Schritt 0 — bestehende Wetterstation „Erder" als Basis übernehmen (Pflicht, zuerst):**
> Der Umweltmonitor wird **nicht neu erfunden**, sondern erbt die bewährte Lösung der Station
> als **Default**. Abweichen nur dort, wo ein Sensor/Aktor es physikalisch erzwingt — und dann
> begründet. Ziel: **Konsistenz** (gleicher Standortbereich, gleiche Baulogik, gleicher Datenfluss).
> - **Bekannt/bewährt (Datenweg 1:1 übernehmen, nicht ändern):** ESP32-C3; ~10-Min-Takt; POST an
>   n8n `https://<N8N_HOST>`; Sheet „<SHEET_DOC>", Tab
>   `<SHEET_TAB>`; Live-Webhook `…/webhook/wetter-latest`; vorhandene Widgets.
> - **Physisch zuerst an der realen Station abnehmen (NICHT annehmen):** Gehäusebauform & Material,
>   Montage (Mast/Höhe), genauer Standort + Ausrichtung, Versorgung (Netz/Solar+Akku), WLAN-Lage/RSSI.
> - Erst **danach** prüfen, was die neuen Sensoren/der Servo am Basis-Aufbau ändern müssen.
>
> **Feste Bestückung (gegeben, NICHT zur Debatte):**
> - Bodenfeuchte — HW-390 kapazitiv v2.0 — analog (via ADS1115) — % rel.
> - Wasserstand/Nässe — HW-038 (Leitfähigkeit) — analog (via ADS1115) — rel. Pegel
> - Regen — Raindrop-Modul (LM393), 3,3–5 V — digital DO (alt. AO) — Index (**kein mm**)
> - Schallpegel — Elektret + MAX9814 (AGC) — analog, nativer C3-ADC — rel. dB
> - Gas CO/brennbar/VOC — MiCS-5524 (MOS, Heizer) — analog (via ADS1115) — rel.
> - UV — GUVA-S12SD (Modul ~0–1 V) — analog (via ADS1115) — UV-Index n. Kennlinie
> - Farbe/Helligkeit — TCS34725 (RGB+Clear) — I²C — RGB / grobe Lux
> - **MCU ESP32-C3 + ADS1115** (Analog-Frontend).
> - **Aktorik: Servo SG90 9g** zum Leeren des Regenbehälters (mit Trichter/Sammelbehälter →
>   **mm nach Kalibrierung**: `mm = (Leerungen × Dump-Volumen) / Fangfläche`).
>
> **Aufgabe — drei verknüpfte Entscheidungen, jeweils ausgehend vom Basis-Aufbau:**
> 1. **Pro Sensor/Aktor die physikalischen Aufstell-Anforderungen ableiten:** Luftaustausch oder
>    geschlossen? Sonnen-/Strahlungsschutz? Regen-/Kondensschutz? Pflicht-Orientierung (UV/Farbe
>    zum Himmel)? Betriebsgrenzen (Temp/Feuchte/IP)? Verfälschende Störquellen?
> 2. **Konflikte auflösen** (erwartbar: Temp/Feuchte wollen Schatten+Durchzug+Regenschutz; UV/Farbe
>    + Regensammler wollen freie Himmelssicht; Gas/Mic wollen freie Luft; Boden/Wasser wollen
>    Erdboden). Daraus die **Gehäuse-Architektur** entscheiden — ausgehend vom Stations-Gehäuse:
>    reicht dessen Bauform, oder braucht es einen **geteilten Aufbau** (passiv belüfteter
>    Strahlungsschutz „Stevenson-/Lamellen-Shield"; himmelsorientierte Sensoren oben; Regensammler
>    + Servo separat; Bodensonden per Kabel/ESP-NOW-Node im Erdreich)?
> 3. **Standort/Position festlegen** — bevorzugt im selben Bereich wie die Station.
>
> **Worauf besonders zu achten ist (Prüfpunkte):**
> - **Eigenerwärmung = Falle Nr. 1:** ESP32 + WLAN + Versorgung heizen das Gehäuse → mehrere °C
>   Bias bei Temp/Feuchte. Elektronik thermisch von Sensoren trennen (eigenes Fach, Abstand, Wärme
>   weg, niedriger Duty-Cycle). **Bis quantifiziert gelten T/H als nicht vertrauenswürdig.**
> - **Strahlungsschutz** (Lamellen, weiß/reflektierend) mit freiem Durchzug.
> - **Regensammler + Servo:** Trichter (definierte Fangfläche), Sammelbehälter, Kippmechanik/Klappe;
>   **Servo SG90 nicht wetterfest** → schützen oder MG90-wasserfest; geringes Drehmoment (~1,8 kg·cm)
>   → Mechanik balanciert auslegen; **eigene 5-V-Versorgung** mit Stützkondensator; **Frostlage** bedenken.
> - **Korrosion:** HW-038 und Raindrop sind leitfähig → so montieren/bestromen, dass kein Dauerkontakt
>   unter Spannung entsteht (Firmware gated, aber Montage entsprechend).
> - **Kondens/Feuchte** verfälscht Sensoren → Membran/PTFE-Druckausgleich statt komplett dicht.
> - **Material/IP:** UV-/witterungsfest (ASA, PETG, PC — **kein PLA**); Elektronikfach ~IP65 mit
>   Kabelverschraubungen, Sensorfach belüftet.
> - **Standort-Regeln (Orientierung, nicht streng normativ):** Lufttemperatur ~1,25–2 m über Gras,
>   Abstand zu Wänden/Pflaster/Gebäuden/Bäumen; Regensammler offen + Rand waagerecht; UV/Farbe freie
>   Himmelssicht; weg von Kamin/Abluft/Parkplatz/Grill (CO₂/PM-Kontamination); Solarpanel ggf. Süden.
> - **WLAN-RSSI am Kandidatenstandort messen** (zur n8n/zum Router) — bei schwachem Pegel externe
>   Antenne/Repeater oder ESP-NOW-Node + Gateway.
> - **Verteilter Aufbau (ESP-NOW) prüfen:** Wegen der Platzkonflikte drängt sich ein Boden-Node
>   (Bodenfeuchte/Wasser/Regen) + Luft-/Himmel-Node mit Gateway-ESP32 auf. Abwägen vs. eine
>   verkabelte Box.
>
> **Vorgehen (bitte einhalten):**
> 1. **Vor Empfehlungen einen Testplan** (`TESTPLAN.md`): abgeschirmt vs. Referenz-Thermometer,
>    Eigenerwärmungs-Delta, Kondens-Check, RSSI am Standort, Regensammler-Fang-/Dump-Test, Servo-Schutz.
> 2. **Erst Basis (Schritt 0) erfassen**, dann Hardware-/Standort-Realität klären (Montage, Höhe,
>    Stromoption inkl. 5 V für Servo, Fotos/Maße/Himmelsrichtungen, WLAN-Lage). Fehlt etwas, gezielt
>    nachfragen statt annehmen.
> 3. **Erst kurzer Plan** (Gehäuse-Architektur als Delta zur Station, Fächeraufteilung, Sensorpositionen
>    + Pflicht-Orientierungen, Regensammler/Servo-Mechanik, Material, Montage, Standort), dann
>    Umsetzungsdetails (Maße, Druckteile/Bauteilliste, Montageskizze).
> 4. **Bewiesen vs. vermutet strikt trennen.** Festgelegte Werte (Bestückung, MCU, ADS1115, Datenweg,
>    Pins, Intervalle) nicht eigenmächtig ändern; aus der Station übernommene Werte kennzeichnen.
> 5. **Projektdateien** unter `…\02_PROJECTS\WetterStudioTeam\UmweltMonitor\gehaeuse-standort\…` ablegen.
>
> **Fertig, wenn:**
> 1. Der **Basis-Aufbau der Station** dokumentiert ist und klar steht, was übernommen und was
>    (begründet) abgewichen wird.
> 2. Für **jeden** Sensor/Aktor die Aufstell-Anforderung dokumentiert und alle Konflikte aufgelöst sind.
> 3. Eine begründete **Gehäuse-Architektur** (ein/geteilt bzw. verteilt, Fächer, Strahlungsschutz,
>    Regensammler+Servo-Mechanik, Material, IP) mit Sensorpositionen + Pflicht-Orientierungen +
>    Montageskizze/Bauteilliste vorliegt.
> 4. Ein **Standort + Position** festgelegt ist (Höhe, Ausrichtung, Abstände, begründet) inkl.
>    gemessenem WLAN-RSSI und geklärter Versorgung (inkl. 5 V für Servo).
> 5. Aufbau und Reaktions-/Plausibilitätstests in `TESTPLAN.md` belegt sind (insb. Eigenerwärmung,
>    Strahlungsschutz, Regensammler-Dump).
