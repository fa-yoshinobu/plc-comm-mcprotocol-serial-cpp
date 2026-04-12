@echo off
setlocal

set "STATUS=0"
set "INTERACTIVE=0"
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "REPO_ROOT=%%~fI"
set "BUILD_DIR=%REPO_ROOT%\build_win"
set "RUN_PLATFORMIO=0"
set "CMAKE_GENERATOR_ARG="

if "%~1"=="" set "INTERACTIVE=1"

:parse
if "%~1"=="" goto main
if /i "%~1"=="--help" goto usage
if /i "%~1"=="--with-platformio" (
  set "RUN_PLATFORMIO=1"
  shift
  goto parse
)
if /i "%~1"=="--build-dir" (
  if "%~2"=="" goto usage
  set "BUILD_DIR=%~f2"
  shift
  shift
  goto parse
)
>&2 echo Unknown argument: %~1
goto usage

:main
pushd "%REPO_ROOT%" >nul

call :prepend_common_tool_paths
call :find_tool cmake.exe CMAKE_EXE
if errorlevel 1 goto fail
call :find_tool ctest.exe CTEST_EXE
if errorlevel 1 goto fail
call :find_python
if errorlevel 1 goto fail

where ninja >nul 2>nul
if not errorlevel 1 set "CMAKE_GENERATOR_ARG=-G Ninja"

echo [ci] Configure
call "%CMAKE_EXE%" -S "%REPO_ROOT%" -B "%BUILD_DIR%" %CMAKE_GENERATOR_ARG%
if errorlevel 1 goto fail

echo [ci] Build
call "%CMAKE_EXE%" --build "%BUILD_DIR%"
if errorlevel 1 goto fail

echo [ci] Test
call "%CTEST_EXE%" --test-dir "%BUILD_DIR%" --output-on-failure
if errorlevel 1 goto fail

echo [ci] Docs and Markdown links
call "%CMAKE_EXE%" --build "%BUILD_DIR%" --target docs
if errorlevel 1 goto fail

call "%PYTHON_EXE%" %PYTHON_ARGS% "%REPO_ROOT%\scripts\check_markdown_links.py"
if errorlevel 1 goto fail

if "%RUN_PLATFORMIO%"=="1" (
  call :run_platformio
  if errorlevel 1 goto fail
)

echo [ci] OK
goto finish

:prepend_common_tool_paths
if exist "C:\msys64\ucrt64\bin" set "PATH=C:\msys64\ucrt64\bin;%PATH%"
if exist "C:\Program Files\CMake\bin" set "PATH=C:\Program Files\CMake\bin;%PATH%"
if exist "C:\Program Files\Doxygen\bin" set "PATH=C:\Program Files\Doxygen\bin;%PATH%"
exit /b 0

:find_tool
set "%~2="
for %%I in (%~1) do if not "%%~$PATH:I"=="" (
  set "%~2=%%~$PATH:I"
  exit /b 0
)
>&2 echo %~1 not found.
exit /b 1

:find_python
set "PYTHON_EXE="
set "PYTHON_ARGS="
py -3 -c "import sys; print(sys.version)" >nul 2>nul
if not errorlevel 1 (
  set "PYTHON_EXE=py"
  set "PYTHON_ARGS=-3"
  exit /b 0
)

python -c "import sys; print(sys.version)" >nul 2>nul
if not errorlevel 1 (
  set "PYTHON_EXE=python"
  exit /b 0
)

>&2 echo Python not found.
exit /b 1

:run_platformio
call :find_tool pio.exe PIO_EXE
if errorlevel 1 (
  >&2 echo PlatformIO not found. Install pio or omit --with-platformio.
  exit /b 1
)

for %%E in (
  native-example
  rpipico-arduino-example
  esp32-c3-devkitm-1-example
  native-example-ultra-minimal
  rpipico-arduino-example-ultra-minimal
  esp32-c3-devkitm-1-example-ultra-minimal
  rpipico-arduino-uart-example
  esp32-c3-devkitm-1-uart-example
  mega2560-arduino-uart-example
) do (
  echo [ci] PlatformIO %%E
  call "%PIO_EXE%" run -e %%E
  if errorlevel 1 exit /b 1
)

exit /b 0

:fail
set "STATUS=%errorlevel%"

:finish
if defined REPO_ROOT if exist "%REPO_ROOT%" popd >nul 2>nul
if "%INTERACTIVE%"=="1" pause
exit /b %STATUS%

:usage
echo usage: %~nx0 [--build-dir DIR] [--with-platformio]
set "STATUS=2"
goto finish
