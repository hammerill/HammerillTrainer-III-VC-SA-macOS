#!/bin/bash
# Installs Hammerill Trainer into the macOS version of GTA Vice City.
#
#   ./install.sh [path/to/HammerillTrainerVC.dylib] [path/to/GTA Vice City.app]
#
# Copies the plugin into the app's Frameworks folder and makes the app's
# launcher script load it. Run uninstall.sh to undo.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
DYLIB="${1:-}"
if [ -z "$DYLIB" ]; then
    # Next to this script in a release download, or in the build folder.
    for candidate in "$HERE/HammerillTrainerVC.dylib" "$HERE/../build/HammerillTrainerVC.dylib"; do
        if [ -f "$candidate" ]; then DYLIB="$candidate"; break; fi
    done
fi
APP="${2:-${GTAVC_APP:-/Applications/GTA Vice City.app}}"

LAUNCHER="$APP/Contents/MacOS/reVC"
BACKUP="$LAUNCHER.hammerilltrainer-backup"
TARGET="$APP/Contents/Frameworks/HammerillTrainerVC.dylib"

fail() { echo "error: $*" >&2; exit 1; }

[ -n "$DYLIB" ] && [ -f "$DYLIB" ] || fail "HammerillTrainerVC.dylib not found. Build it first (see README) or pass its path."
[ -d "$APP" ] || fail "\"$APP\" not found. Pass the app's path as the second argument."
[ -f "$LAUNCHER" ] || fail "\"$LAUNCHER\" not found. Is this the macOS reVC build of Vice City?"

# The launcher script runs the real game executable; find its name.
GAME_EXE="$(cd "$APP/Contents/MacOS" && for f in *; do
    [ "$f" != reVC ] && [[ "$f" != *.hammerilltrainer-backup ]] && file "$f" | grep -q 'Mach-O.*executable' && echo "$f" && break
done || true)"
[ -n "$GAME_EXE" ] || fail "couldn't find the game executable in \"$APP/Contents/MacOS\"."

echo "Installing plugin to $TARGET"
cp "$DYLIB" "$TARGET"
# A downloaded copy carries a quarantine flag that would stop it from loading.
xattr -d com.apple.quarantine "$TARGET" 2>/dev/null || true
codesign --force --sign - "$TARGET" >/dev/null 2>&1 || true

if [ ! -f "$BACKUP" ]; then
    cp "$LAUNCHER" "$BACKUP"
    echo "Saved the original launcher as $(basename "$BACKUP")"
fi

cat > "$LAUNCHER" <<EOF
#!/bin/bash
# Patched by the Hammerill Trainer installer. The original is reVC.hammerilltrainer-backup;
# run uninstall.sh (or copy it back) to remove the trainer.
HERE="\$(cd "\$(dirname "\$0")" && pwd)"
cd "\$HERE/../Resources/"
TRAINER="\$HERE/../Frameworks/HammerillTrainerVC.dylib"
if [ -f "\$TRAINER" ]; then
    export DYLD_INSERT_LIBRARIES="\$TRAINER"
fi
exec "\$HERE/$GAME_EXE" "\$@"
EOF
chmod +x "$LAUNCHER"

echo "Done. Start GTA Vice City as usual and press Left Ctrl + T in game."
