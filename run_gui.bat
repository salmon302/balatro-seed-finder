@echo off
setlocal
cd /d "%~dp0"

REM Launch the Tkinter GUI
where py >nul 2>nul
if %errorlevel%==0 (
  py -3 gui\seed_finder_gui.py
) else (
  python gui\seed_finder_gui.py
)
