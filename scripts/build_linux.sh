#!/usr/bin/env bash
set -e

echo "=========================================================="
echo "  Building 'Re-Feedback' by Omega Dumpster for Linux"
echo "=========================================================="

BUILD_DIR="build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
ninja

echo ""
echo "Running DSP Tests..."
./DSPTests

echo ""
echo "=========================================================="
echo "  Build & Verification Completed Successfully!"
echo "  Standalone App: $BUILD_DIR/ReFeedback_artefacts/Release/Standalone/Re-Feedback"
echo "  VST3 Plugin:    $BUILD_DIR/ReFeedback_artefacts/Release/VST3/Re-Feedback.vst3"
echo "=========================================================="
