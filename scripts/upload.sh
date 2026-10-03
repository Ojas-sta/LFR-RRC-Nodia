#!/usr/bin/env bash
# ==============================================================================
# LFR-RRC-Nodia Firmware Build and Upload Utility
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

echo "=================================================="
echo "  LFR-RRC-Nodia Build & Flash Tool"
echo "=================================================="

# 1. Check for PlatformIO CLI
PIO_CMD=""
if command -v pio &> /dev/null; then
    PIO_CMD="pio"
elif command -v platformio &> /dev/null; then
    PIO_CMD="platformio"
else
    echo "ERROR: PlatformIO CLI ('pio' or 'platformio') is not installed or not in PATH."
    echo "Please install PlatformIO: https://platformio.org/install/cli"
    echo "Or run: pip install -U platformio"
    exit 1
fi

PORT="$1"

# 2. Build the project
echo ""
echo "--> Compiling firmware..."
if ! "$PIO_CMD" run; then
    echo "ERROR: Build failed. Check the compiler output above."
    exit 2
fi

# 3. Upload firmware
echo ""
if [ -n "$PORT" ]; then
    echo "--> Uploading firmware to specified port: $PORT ..."
    if ! "$PIO_CMD" run -t upload --upload-port "$PORT"; then
        echo "ERROR: Upload failed on port $PORT."
        exit 3
    fi
else
    echo "--> Uploading firmware (auto-detecting serial port)..."
    if ! "$PIO_CMD" run -t upload; then
        echo "ERROR: Upload failed. Please specify the serial port explicitly:"
        echo "Usage: $0 [SERIAL_PORT]"
        echo "Example: $0 /dev/cu.usbserial-0001"
        exit 3
    fi
fi

echo ""
echo "=================================================="
echo "  Firmware Upload Successful!"
echo "  To monitor serial output, run:"
echo "    $PIO_CMD device monitor -b 115200"
echo "=================================================="
