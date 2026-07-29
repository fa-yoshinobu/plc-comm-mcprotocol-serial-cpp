@echo off
setlocal

set "RUN_PLATFORMIO=0"
set "VERSION="
set "JSON_VERSION="

:parse
if "%~1"=="" goto main
if /i "%~1"=="--help" goto usage
if /i "%~1"=="--with-platformio" (
  set "RUN_PLATFORMIO=1"
  shift
  goto parse
)
>&2 echo Unknown argument: %~1
goto usage

:main
for /f "tokens=1,2 delims==" %%A in (library.properties) do (
  if /I "%%A"=="version" set "VERSION=%%B"
)
if not defined VERSION (
  echo [ERROR] Failed to read version from library.properties.
  exit /b 1
)

for /f "usebackq delims=" %%V in (`powershell -NoProfile -Command "(Get-Content library.json -Raw | ConvertFrom-Json).version"`) do set "JSON_VERSION=%%V"
if not defined JSON_VERSION (
  echo [ERROR] Failed to read version from library.json.
  exit /b 1
)

echo ===================================================
echo [RELEASE] MC Protocol Serial C++ release check
echo ===================================================

echo [1/5] Syncing mirrored release metadata...
python scripts\sync_release_metadata.py
if %errorlevel% neq 0 (
  echo [ERROR] Release metadata synchronization failed.
  exit /b %errorlevel%
)
set "VERSION="
set "JSON_VERSION="
for /f "tokens=1,2 delims==" %%A in (library.properties) do (
  if /I "%%A"=="version" set "VERSION=%%B"
)
if not defined VERSION (
  echo [ERROR] Failed to read version from library.properties.
  exit /b 1
)
for /f "usebackq delims=" %%V in (`powershell -NoProfile -Command "(Get-Content library.json -Raw | ConvertFrom-Json).version"`) do set "JSON_VERSION=%%V"
if not defined JSON_VERSION (
  echo [ERROR] Failed to read version from library.json.
  exit /b 1
)

echo [2/5] Refreshing generated API reference...
python scripts\update_api_reference.py
if %errorlevel% neq 0 (
  echo [ERROR] API reference generation failed.
  exit /b %errorlevel%
)

echo [3/5] Checking manifest versions...
if not "%VERSION%"=="%JSON_VERSION%" (
  echo [ERROR] library.properties version %VERSION% does not match library.json version %JSON_VERSION%.
  exit /b 1
)

echo [4/5] Running host CI gate...
call run_ci.bat --build-dir build_win
if %errorlevel% neq 0 (
  echo [ERROR] CI failed.
  exit /b %errorlevel%
)

if "%RUN_PLATFORMIO%"=="1" (
  echo [platformio] Checking registry duplicate and package build...
  powershell -NoProfile -ExecutionPolicy Bypass -File scripts\check_registry_duplicate.ps1 -Registry platformio -Package fa-yoshinobu/mcprotocol-serial-cpp -VersionSource library-properties -ManifestPath library.properties -CompareSource library-json -CompareManifestPath library.json
  if errorlevel 1 (
    echo [ERROR] PlatformIO registry check failed.
    exit /b 1
  )

  call run_ci.bat --build-dir build_win --with-platformio
  if errorlevel 1 (
    echo [ERROR] PlatformIO sample gate failed.
    exit /b 1
  )

  if not exist release-artifacts mkdir release-artifacts
  if exist "%USERPROFILE%\.platformio\penv\Scripts\pio.exe" (
    "%USERPROFILE%\.platformio\penv\Scripts\pio.exe" pkg pack --output release-artifacts
  ) else (
    pio pkg pack --output release-artifacts
  )
  if errorlevel 1 (
    echo [ERROR] PlatformIO pack failed.
    exit /b 1
  )
)

echo [5/5] Checking tracked archive contents...
if not exist release-artifacts mkdir release-artifacts
git archive --format=zip --output "release-artifacts\mcprotocol-serial-cpp-v%VERSION%.zip" HEAD
if %errorlevel% neq 0 (
  echo [ERROR] Failed to build release archive.
  exit /b %errorlevel%
)
python scripts\check_release_archive.py "release-artifacts\mcprotocol-serial-cpp-v%VERSION%.zip"
if errorlevel 1 (
  echo [ERROR] Release archive content check failed.
  exit /b 1
)

echo ===================================================
echo [SUCCESS] Release check passed.
echo ===================================================
endlocal
exit /b 0

:usage
echo usage: %~nx0 [--with-platformio]
exit /b 2
