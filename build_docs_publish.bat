@echo off
setlocal
set "DOCS_OUTPUT_DIR=build\docs"
set "TEMP_DOXYFILE=.tmp_doxyfile"

echo ===================================================
echo [DOCS] Publishing MC Protocol Serial C++ Documentation...
echo [DOCS] Output: %DOCS_OUTPUT_DIR%
echo ===================================================

if exist "%DOCS_OUTPUT_DIR%" (
    rmdir /s /q "%DOCS_OUTPUT_DIR%"
)

powershell -NoProfile -Command "$content = Get-Content 'Doxyfile'; $content = $content -replace '^OUTPUT_DIRECTORY\\s*=.*$', 'OUTPUT_DIRECTORY       = \"%DOCS_OUTPUT_DIR%\"'; Set-Content -Path '%TEMP_DOXYFILE%' -Value $content"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to prepare temporary Doxygen configuration.
    exit /b 1
)

call scripts\run_doxygen.bat "%TEMP_DOXYFILE%"

if %errorlevel% equ 0 (
    del /q %TEMP_DOXYFILE% >nul 2>&1
    echo [SUCCESS] Documentation published at: %DOCS_OUTPUT_DIR%/api/index.html
    exit /b 0
) else (
    del /q %TEMP_DOXYFILE% >nul 2>&1
    echo [ERROR] Doxygen generation failed.
    exit /b %errorlevel%
)

endlocal
