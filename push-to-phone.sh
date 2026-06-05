#!/bin/bash
# push-to-phone.sh
# Pushes a .pbw to the watch via Developer Connection (Dev Connection)
#
# Usage:
#   ./push-to-phone.sh                    # Uses default phone IP
#   ./push-to-phone.sh path/to/file.pbw   # Specific pbw file
#
# Default phone IP (Tailscale / local network)
PHONE_IP="${PEBBLE_PHONE:-192.168.68.55}"

PBW_FILE="$1"

if [ -z "$PBW_FILE" ]; then
    # Try to find the most recent .pbw in build/
    PBW_FILE=$(ls -t build/*.pbw 2>/dev/null | head -1)
fi

if [ -z "$PBW_FILE" ] || [ ! -f "$PBW_FILE" ]; then
    echo "ERROR: No .pbw file found."
    echo "Usage: $0 [path/to/watchface.pbw]"
    echo "Or place the .pbw in the build/ directory."
    exit 1
fi

echo "==> Pushing $PBW_FILE to watch via $PHONE_IP (Developer Connection)..."
pebble install --phone "$PHONE_IP" "$PBW_FILE"

echo "==> Done. Watchface should now be active on the watch."