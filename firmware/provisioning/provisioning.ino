// Einmaliger NVS-Provisioner fuer den UmweltMonitor.
// Schreibt Secrets in den NVS-Namespace "um" — denselben, den loadSecrets() der Hauptfirmware liest.
// Werte landen NUR im NVS (eigene Flash-Partition, ueberlebt App-Upload), nie im Quellcode.
//
// Bedienung ueber Seriellen Monitor @115200, Zeilenende senden:
//   set ssid  <wlan-name>
//   set pass  <passwort>
//   set url   <n8n-webhook-url>
//   set hmac  <hmac-key>
//   list      (zeigt Stand; pass/hmac maskiert)
//   clear     (loescht alle Keys im Namespace)
#include <Preferences.h>

Preferences prefs;
String line;

void show() {
  prefs.begin("um", true);
  Serial.printf("NVS: ssid=%s pass=%s url=%s hmac=%s seq=%u\n",
    prefs.getString("ssid", "(leer)").c_str(),
    prefs.getString("pass", "").length() ? "***gesetzt***" : "(leer)",
    prefs.getString("url",  "(leer)").c_str(),
    prefs.getString("hmac", "").length() ? "***gesetzt***" : "(leer)",
    prefs.getUInt("seq", 0));
  prefs.end();
}

void apply(String s) {
  s.trim();
  if (s == "list")  { show(); return; }
  if (s == "clear") { prefs.begin("um", false); prefs.clear(); prefs.end(); Serial.println(F("cleared")); show(); return; }
  if (s.startsWith("set ")) {
    s = s.substring(4); s.trim();
    int sp = s.indexOf(' ');
    if (sp < 1) { Serial.println(F("usage: set <key> <value>")); return; }
    String k = s.substring(0, sp);
    String v = s.substring(sp + 1);
    prefs.begin("um", false);
    prefs.putString(k.c_str(), v);
    prefs.end();
    Serial.printf("OK set %s (%d Zeichen)\n", k.c_str(), v.length());
    return;
  }
  Serial.println(F("? Befehle: set <key> <value> | list | clear"));
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n=== NVS-Provisioner (Namespace 'um') ==="));
  Serial.println(F("Befehle: set <key> <value> | list | clear"));
  show();
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') { if (line.length()) { apply(line); line = ""; } }
    else line += c;
  }
}
