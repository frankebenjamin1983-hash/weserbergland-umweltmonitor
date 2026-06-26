@echo off
rem UmweltMonitor-Panel-Launcher: startet den lokalen Static-Server (falls noetig)
rem und oeffnet das Panel als Edge-App-Fenster (Web-Serial braucht localhost, nicht file://).
title UmweltMonitor Panel Launcher
set "PORT=8765"
set "PDIR=C:\Users\bfran\WORK\02_PROJECTS\WetterStudioTeam\UmweltMonitor\panel"
set "URL=http://localhost:%PORT%/umweltmonitor_panel.html"

rem Laeuft schon ein Server auf dem Port? Dann nicht neu starten.
netstat -ano | findstr ":%PORT% " >nul 2>&1
if errorlevel 1 (
  start "UmweltMonitor-Server (dieses Fenster schliessen = Panel-Server stoppen)" /min python -m http.server %PORT% -d "%PDIR%"
  rem ~1s warten, bis der Server oben ist
  ping -n 2 127.0.0.1 >nul
)

rem Panel oeffnen (Edge = Web-Serial-faehig). Fallback: Standardbrowser.
start "" msedge "--app=%URL%" || start "" "%URL%"
exit
