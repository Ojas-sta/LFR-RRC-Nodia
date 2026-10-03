#!/usr/bin/env bash
# ==============================================================================
# LFR-RRC-Nodia One-Click Firmware Build & Upload (Linux / macOS)
# ==============================================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "$SCRIPT_DIR/scripts/upload.sh" "$@"
