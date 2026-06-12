@echo off
setlocal

echo ===================================================
echo [RELEASE] MC Protocol Serial C++ release check
echo ===================================================

echo [1/3] Checking registry version...
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\check_registry_duplicate.ps1 -Registry platformio -Package fa-yoshinobu/mcprotocol-serial-cpp -VersionSource library-properties -ManifestPath library.properties -CompareSource library-json -CompareManifestPath library.json
if %errorlevel% neq 0 (
    echo [ERROR] Release version check failed.
    exit /b %errorlevel%
)

echo [2/3] Running CI...
call scripts\run_ci.bat
if %errorlevel% neq 0 (
    echo [ERROR] CI failed.
    exit /b %errorlevel%
)

echo [3/3] Packing PlatformIO package...
if not exist release-artifacts mkdir release-artifacts
set "PIO_EXE=pio"
if exist "%USERPROFILE%\.platformio\penv\Scripts\pio.exe" set "PIO_EXE=%USERPROFILE%\.platformio\penv\Scripts\pio.exe"
"%PIO_EXE%" pkg pack --output release-artifacts
if %errorlevel% neq 0 (
    echo [ERROR] PlatformIO pack failed.
    exit /b %errorlevel%
)

echo ===================================================
echo [SUCCESS] Release check passed.
echo ===================================================
endlocal
