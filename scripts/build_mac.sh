#!/usr/bin/env bash
set -e

echo "=========================================================="
echo "  Building 'Re-Feedback' by Omega Dumpster for macOS (AU / VST3 / App)"
echo "=========================================================="

BUILD_DIR="build_mac"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure for macOS Universal Binary (arm64 + x86_64)
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15"

# Build all targets
cmake --build . --config Release -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "=========================================================="
echo "  Build Completed Successfully!"
echo "=========================================================="

# Destination directories on macOS
AU_DEST="$HOME/Library/Audio/Plug-Ins/Components"
VST3_DEST="$HOME/Library/Audio/Plug-Ins/VST3"

mkdir -p "$AU_DEST" "$VST3_DEST"

# Copy AU and VST3 if running on macOS
if [ "$(uname)" = "Darwin" ]; then
    echo "Installing Audio Unit (.component) to $AU_DEST..."
    cp -R "ReFeedback_artefacts/Release/AU/Re-Feedback.component" "$AU_DEST/" 2>/dev/null || \
    cp -R "ReFeedback_artefacts/AU/Re-Feedback.component" "$AU_DEST/" 2>/dev/null || true

    echo "Installing VST3 (.vst3) to $VST3_DEST..."
    cp -R "ReFeedback_artefacts/Release/VST3/Re-Feedback.vst3" "$VST3_DEST/" 2>/dev/null || \
    cp -R "ReFeedback_artefacts/VST3/Re-Feedback.vst3" "$VST3_DEST/" 2>/dev/null || true

    echo "Restart your DAW (Logic Pro, Ableton Live, Reaper, GarageBand, FL Studio, Bitwig) to use Re-Feedback!"
else
    echo "Compiled binaries are located in $BUILD_DIR/ReFeedback_artefacts/"
fi
