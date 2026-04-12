@echo off
setlocal

set "STATUS=0"
set "INTERACTIVE=0"
set "PUSHD_DONE=0"
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "REPO_ROOT=%%~fI"

if not "%~1"=="" if /i "%~1"=="--help" (
  >&2 echo usage: %~nx0 [doxyfile]
  exit /b 2
)

if not "%~1"=="" if not "%~2"=="" (
  >&2 echo usage: %~nx0 [doxyfile]
  exit /b 2
)

if "%~1"=="" (
  set "INTERACTIVE=1"
  set "DOXYFILE=%REPO_ROOT%\Doxyfile"
) else (
  set "DOXYFILE=%~1"
)

for %%I in ("%DOXYFILE%") do (
  set "DOXYFILE=%%~fI"
  set "DOXYFILE_DIR=%%~dpI"
)

if not exist "%DOXYFILE%" (
  >&2 echo Doxyfile not found: %DOXYFILE%
  set "STATUS=1"
  goto finish
)

call :prepend_common_tool_paths
call :find_doxygen
if errorlevel 1 (
  set "STATUS=1"
  goto finish
)

pushd "%DOXYFILE_DIR%" >nul
if errorlevel 1 (
  >&2 echo Failed to change directory: %DOXYFILE_DIR%
  set "STATUS=1"
  goto finish
)
set "PUSHD_DONE=1"

if /i "%DOXYFILE%"=="%REPO_ROOT%\Doxyfile" (
  if not exist "docs\api" mkdir "docs\api" >nul 2>nul
)

echo [doxygen] %DOXYFILE%
"%DOXYGEN_EXE%" "%DOXYFILE%"
set "STATUS=%errorlevel%"

goto finish

:prepend_common_tool_paths
if exist "C:\msys64\ucrt64\bin" set "PATH=C:\msys64\ucrt64\bin;%PATH%"
if exist "C:\Program Files\CMake\bin" set "PATH=C:\Program Files\CMake\bin;%PATH%"
if exist "C:\Program Files\Doxygen\bin" set "PATH=C:\Program Files\Doxygen\bin;%PATH%"
exit /b 0

:find_doxygen
set "DOXYGEN_EXE="

if exist "C:\Program Files\Doxygen\bin\doxygen.exe" (
  set "DOXYGEN_EXE=C:\Program Files\Doxygen\bin\doxygen.exe"
  exit /b 0
)

if exist "%REPO_ROOT%\.tools\doxygen\bin\doxygen.exe" (
  set "DOXYGEN_EXE=%REPO_ROOT%\.tools\doxygen\bin\doxygen.exe"
  exit /b 0
)

if exist "%REPO_ROOT%\.tools\doxygen\usr\bin\doxygen.exe" (
  set "DOXYGEN_EXE=%REPO_ROOT%\.tools\doxygen\usr\bin\doxygen.exe"
  exit /b 0
)

for %%I in (doxygen.exe) do if not "%%~$PATH:I"=="" (
  set "DOXYGEN_EXE=%%~$PATH:I"
  exit /b 0
)

>&2 echo Doxygen not found.
>&2 echo Install it system-wide or place a local bundle under .tools\doxygen\.
exit /b 1

:finish
if "%PUSHD_DONE%"=="1" popd >nul
if "%INTERACTIVE%"=="1" pause
exit /b %STATUS%
