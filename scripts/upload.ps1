# ==============================================================================
# LFR-RRC-Nodia Build & Flash Tool (PowerShell)
# ==============================================================================
param(
    [string]$Port = ""
)

$ErrorActionPreference = "Stop"

# Navigate to project root (one level up from scripts/)
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -Path $projectRoot

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "   LFR-RRC-Nodia Build & Flash Tool (PowerShell)  " -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# 1. Check for PlatformIO CLI
$pioCmd = Get-Command pio -ErrorAction SilentlyContinue
if (-not $pioCmd) {
    $pioCmd = Get-Command platformio -ErrorAction SilentlyContinue
}

if (-not $pioCmd) {
    Write-Host ""
    Write-Host "ERROR: PlatformIO CLI ('pio' or 'platformio') is not installed or not in PATH." -ForegroundColor Red
    Write-Host "Please install PlatformIO: https://platformio.org/install/cli"
    Write-Host "Or run: pip install -U platformio"
    exit 1
}

# 2. Compile firmware
Write-Host ""
Write-Host "--> Compiling firmware..." -ForegroundColor Green
try {
    & $pioCmd.Source run
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Build failed. Check compiler output above." -ForegroundColor Red
        exit $LASTEXITCODE
    }
} catch {
    Write-Host "ERROR: Failed to run PlatformIO compilation." -ForegroundColor Red
    exit 1
}

# 3. Upload firmware
Write-Host ""
if ([string]::IsNullOrWhiteSpace($Port)) {
    Write-Host "--> Uploading firmware (auto-detecting serial port)..." -ForegroundColor Green
    & $pioCmd.Source run -t upload
} else {
    Write-Host "--> Uploading firmware to specified port: $Port ..." -ForegroundColor Green
    & $pioCmd.Source run -t upload --upload-port $Port
}

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Upload failed. Please check ESP32 USB connection and COM port." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "==================================================" -ForegroundColor Green
Write-Host "   Firmware Upload Successful!                    " -ForegroundColor Green
Write-Host "==================================================" -ForegroundColor Green
