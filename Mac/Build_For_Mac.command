#!/usr/bin/env bash
set -e

# Change to the project directory (parent of Mac/ folder)
DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$DIR"

echo "=========================================================="
echo "  Building Re-Feedback for macOS (AU & VST3 Universal)"
echo "  Developer: Omega Dumpster"
echo "=========================================================="

if ! command -v cmake &> /dev/null; then
    echo ""
    echo "  [!] CMake is required to build on Mac."
    echo "  If you have Homebrew installed, run: brew install cmake"
    echo "  Or download the installer from: https://cmake.org/download/"
    echo ""
    read -p "Press [Enter] to exit..."
    exit 1
fi

BUILD_DIR="build_mac"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure Universal Binary for Apple Silicon (arm64) and Intel (x86_64)
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15"

# Compile all targets using all CPU cores
cmake --build . --config Release -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "=========================================================="
echo "  Copying AU & VST3 bundles directly into 'Mac/' folder..."
echo "=========================================================="

MAC_FOLDER="$DIR/Mac"
mkdir -p "$MAC_FOLDER"

# Copy AU component
if [ -d "ReFeedback_artefacts/Release/AU/Re-Feedback.component" ]; then
    cp -R "ReFeedback_artefacts/Release/AU/Re-Feedback.component" "$MAC_FOLDER/"
elif [ -d "ReFeedback_artefacts/AU/Re-Feedback.component" ]; then
    cp -R "ReFeedback_artefacts/AU/Re-Feedback.component" "$MAC_FOLDER/"
fi

# Copy VST3 bundle
if [ -d "ReFeedback_artefacts/Release/VST3/Re-Feedback.vst3" ]; then
    cp -R "ReFeedback_artefacts/Release/VST3/Re-Feedback.vst3" "$MAC_FOLDER/"
elif [ -d "ReFeedback_artefacts/VST3/Re-Feedback.vst3" ]; then
    cp -R "ReFeedback_artefacts/VST3/Re-Feedback.vst3" "$MAC_FOLDER/"
fi

# Copy Standalone App
if [ -d "ReFeedback_artefacts/Release/Standalone/Re-Feedback.app" ]; then
    cp -R "ReFeedback_artefacts/Release/Standalone/Re-Feedback.app" "$MAC_FOLDER/"
elif [ -d "ReFeedback_artefacts/Standalone/Re-Feedback.app" ]; then
    cp -R "ReFeedback_artefacts/Standalone/Re-Feedback.app" "$MAC_FOLDER/"
fi

echo ""
echo "=========================================================="
echo "  SUCCESS!"
echo "  The 'Mac/' folder is now populated with:"
echo "    - Re-Feedback.component (Audio Unit for Logic, GarageBand, etc.)"
echo "    - Re-Feedback.vst3 (VST3 for Ableton, Reaper, FL Studio, etc.)"
echo "    - Re-Feedback.app (Standalone application)"
echo "    - Install_To_Mac.command (1-click installer for your friend)"
echo ""
echo "  You can now copy the entire 'Mac/' folder onto your USB stick!"
echo "=========================================================="
read -p "Press [Enter] to exit..."
