#!/bin/bash
# Removes Hammerill Trainer from the macOS version of GTA Vice City.
#
#   ./uninstall.sh [path/to/GTA Vice City.app]
set -euo pipefail

APP="${1:-${GTAVC_APP:-/Applications/GTA Vice City.app}}"
LAUNCHER="$APP/Contents/MacOS/reVC"
BACKUP="$LAUNCHER.hammerilltrainer-backup"
TARGET="$APP/Contents/Frameworks/HammerillTrainerVC.dylib"

[ -d "$APP" ] || { echo "error: \"$APP\" not found." >&2; exit 1; }

if [ -f "$BACKUP" ]; then
    mv "$BACKUP" "$LAUNCHER"
    chmod +x "$LAUNCHER"
    echo "Restored the original launcher."
fi
if [ -f "$TARGET" ]; then
    rm "$TARGET"
    echo "Removed $TARGET"
fi
echo "Hammerill Trainer is uninstalled. Your saves and settings were not touched."
