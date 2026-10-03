@echo off
cd /d "%~dp0"
echo ==================================================
echo   LFR-RRC-Nodia Build & Flash Tool (Windows)
echo ==================================================

where pio >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set PIO_CMD=pio
    goto run_pio
)

where platformio >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set PIO_CMD=platformio
    goto run_pio
)

echo ERROR: PlatformIO CLI ('pio' or 'platformio') is not installed or not in PATH.
echo Please install PlatformIO: https://platformio.org/install/cli
echo Or run: pip install -U platformio
pause
exit /b 1

:run_pio
echo.
echo --> Compiling firmware...
%PIO_CMD% run
if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed. Check compiler output above.
    pause
    exit /b 2
)

echo.
if "%~1"=="" (
    echo --> Uploading firmware (auto-detecting serial port)...
    %PIO_CMD% run -t upload
) else (
    echo --> Uploading firmware to specified port: %~1 ...
    %PIO_CMD% run -t upload --upload-port %~1
)

if %ERRORLEVEL% neq 0 (
    echo ERROR: Upload failed. Please check connection and COM port.
    pause
    exit /b 3
)

echo.
echo ==================================================
echo   Firmware Upload Successful!
echo ==================================================
pause
