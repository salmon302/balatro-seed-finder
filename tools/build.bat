@echo off
setlocal ENABLEDELAYEDEXPANSION

REM Windows build script for immolate with selected filter
REM Usage: tools\build.bat <filter_name>

cd /d "%~dp0.." 2>nul

if "%~1"=="" (
  echo Usage: %~nx0 ^<filter_name^>
  echo Available filters:
  for %%F in (filters\*_filter.hpp) do (
    set base=%%~nF
    set base=!base:_filter=!
    echo   !base!
  )
  echo.
  echo Example: %~nx0 perkeo
  exit /b 1
)

set FILTER_NAME=%~1
set FILTER_FILE=filters\%FILTER_NAME%_filter.hpp

if not exist "%FILTER_FILE%" (
  echo Error: Filter file "%FILTER_FILE%" not found!
  echo Available filters:
  for %%F in (filters\*_filter.hpp) do (
    set base=%%~nF
    set base=!base:_filter=!
    echo   !base!
  )
  exit /b 1
)

where g++ >nul 2>nul
if errorlevel 1 (
  echo Error: g++ not found in PATH.
  echo Please install MinGW-w64 and ensure g++.exe is available in your PATH.
  echo For example, install via MSYS2 or https://www.mingw-w64.org/.
  exit /b 1
)

if not exist dist (
  mkdir dist
)

set OUTPUT=dist\immolate_%FILTER_NAME%.exe
set FILTER_DEF=-DSELECTED_FILTER=\"filters\\%FILTER_NAME%_filter.hpp\"

echo Building immolate with filter: %FILTER_NAME%
echo Output: %OUTPUT%

REM Compile with optimization similar to the Unix script
g++ -std=c++14 -O3 -g -DENABLE_LOGS -ffp-contract=off -fexcess-precision=standard %FILTER_DEF% -o "%OUTPUT%" immolate.cpp env.cpp
if errorlevel 1 (
  echo Build failed.
  exit /b 1
)

echo Build successful! Executable: %OUTPUT%
exit /b 0
