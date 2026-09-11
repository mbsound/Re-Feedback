#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"

echo "=========================================================="
echo "  Installing Re-Feedback by Omega Dumpster"
echo "=========================================================="

AU_DEST="$HOME/Library/Audio/Plug-Ins/Components"
VST3_DEST="$HOME/Library/Audio/Plug-Ins/VST3"

mkdir -p "$AU_DEST" "$VST3_DEST"

# Remove quarantine on the local standalone App
if [ -d "$DIR/Re-Feedback.app" ]; then
    echo "Configuring Standalone App permissions..."
    xattr -cr "$DIR/Re-Feedback.app" 2>/dev/null || true
    chmod +x "$DIR/Re-Feedback.app/Contents/MacOS/Re-Feedback" 2>/dev/null || true
    echo "  [OK] Standalone App is ready to launch!"
fi

# 1. Install Audio Unit (.component)
if [ -d "$DIR/Re-Feedback.component" ]; then
    echo "Installing Audio Unit (AU) to $AU_DEST..."
    rm -rf "$AU_DEST/Re-Feedback.component"
    cp -R "$DIR/Re-Feedback.component" "$AU_DEST/"
    xattr -cr "$AU_DEST/Re-Feedback.component" 2>/dev/null || true
    echo "  [OK] Installed Re-Feedback.component"
else
    echo "  [SKIP] Re-Feedback.component not found in this folder."
fi

# 2. Install VST3 (.vst3)
if [ -d "$DIR/Re-Feedback.vst3" ]; then
    echo "Installing VST3 to $VST3_DEST..."
    rm -rf "$VST3_DEST/Re-Feedback.vst3"
    cp -R "$DIR/Re-Feedback.vst3" "$VST3_DEST/"
    xattr -cr "$VST3_DEST/Re-Feedback.vst3" 2>/dev/null || true
    echo "  [OK] Installed Re-Feedback.vst3"
else
    echo "  [SKIP] Re-Feedback.vst3 not found in this folder."
fi

echo ""
echo "=========================================================="
echo "  Installation Complete!"
echo "  - Standalone App: Double-click Re-Feedback.app to run"
echo "  - DAWs: Restart Logic Pro, Ableton Live, Reaper, GarageBand,"
echo "          FL Studio, Studio One, or Bitwig to use the plugin!"
echo "=========================================================="
read -p "Press [Enter] to exit..."
