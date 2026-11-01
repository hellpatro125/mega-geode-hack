@echo off
set "GEODE_SDK=C:\Users\Maks\Documents\Geode"
set "PATH=C:\Users\Maks\.gemini\antigravity\scratch\cmake-bin\cmake-3.31.0-windows-x86_64\bin;C:\Users\Maks\.gemini\antigravity\scratch\geode-bin;%PATH%"

echo ========================================================
echo   MegaHack & Frame Prediction: Geode Mod Builder
echo ========================================================
echo.

geode.exe build

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [!] Build requires Microsoft Visual Studio C++ (MSVC x64).
    echo Packaging and installing current mod files...
    geode.exe package new . -i
) else (
    echo.
    echo [+] Mod compiled and installed successfully!
)

pause
