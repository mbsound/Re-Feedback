<p align="center">
  <img src="Resources/icon.png" alt="Oops! All Feedback" width="220" />
</p>

# ⚡ Re-Feedback
### *The Anti-De-Feedback Audio Plug-in by **Omega Dumpster***

[![Build & Release](https://github.com/mbsound/Re-Feedback/actions/workflows/build.yml/badge.svg)](https://github.com/mbsound/Re-Feedback/actions)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platform](https://img.shields.io/badge/Platform-macOS%20%7C%20Linux-lightgrey.svg)]()
[![Format](https://img.shields.io/badge/Format-AU%20v2%20%7C%20VST3%20%7C%20Standalone-orange.svg)]()

> **AlphaLabs released *De-Feedback* to eliminate feedback. Omega Dumpster built the exact opposite.**  
> **Re-Feedback** is a hyper-configurable physical & electronic recursive feedback synthesizer, overtone resonator, and acoustic howl generator.

---

## 🎸 Overview & Concept

While feedback suppressors use notch filters and AI polarity inversion to kill acoustic howling, **Re-Feedback** models and cultivates the raw physics of acoustic loops, amplifier magnetic pickup coupling, screaming Larsen effects, and tuned harmonic resonance.

It transforms any dry guitar track, vocal line, synth, or drum hit into rich, singing Hendrix/Cobain feedback sustain, or generates self-oscillating acoustic howls completely from silence.

---

## ✨ Key Features

### 1. Multi-Topology Acoustic & Electronic Feedback Modes
- **Amp Coupling**: Simulates electric guitar pickup magnetic and acoustic coupling with an amplifier cabinet (rich singing tube sustain and blooming harmonic overtones).
- **Larsen Howl**: Simulates live PA speaker-to-microphone feedback loops with room boundary reflection delays, air damping, and microphone phase interaction.
- **Comb Resonator**: Musically tuned feedback resonator locking precisely onto harmonic overtone series.
- **Chaos Screamer**: Non-linear cross-modulated feedback with micro-jitter, chaotic frequency flutter, and aggressive ring squeals.

### 2. Auto Pitch Tracking & Harmonic Overtone Matrix
- **Real-Time Pitch Detector**: High-precision autocorrelation McLeod Pitch Method (MPM) tracker detects incoming notes.
- **Harmonic Interval Lock**:
  - **Sub-Octave** ($-12$ st, $0.5\times$)
  - **Fundamental** ($1.0\times$ Unison)
  - **Octave** ($+12$ st, $2.0\times$)
  - **Fifth** ($+19$ st, $3.0\times$ - *classic 3rd harmonic guitar pickup howl*)
  - **2nd Octave** ($+24$ st, $4.0\times$)
  - **Major 3rd** ($+28$ st, $5.0\times$)
  - **Manual / Free**: Freely sweepable $20\text{ Hz} - 20\text{ kHz}$ with microtonal fine detune ($\pm 100$ cents).

### 3. Dynamic Bloom & Transient Control
- **Bloom Rise Time ($5\text{ ms} - 2000\text{ ms}$)**: Allows pick attacks and vocals to ring out cleanly before automatically swelling into screaming feedback during sustained holds.
- **Attack Ducking**: Ducks feedback level during heavy strumming transients.
- **Acoustic Input Coupling**: Natural signal following ensures the plugin stays silent when your instrument is muted/unplugged.

### 4. "ONLY FEEDBACK" & Routing Engine
- **`ONLY FEEDBACK` Mode**: Completely isolates and mutes the dry input, outputting 100% pure feedback resonance.
- **Dry/Wet Gain**: Precision level mixing.

### 5. Exciters & Squeal Trigger
- **Big "SQUEAL!" Stomp Trigger**: Injects an acoustic chirp/impulse into the loop to instantly ignite screaming feedback on silent tracks.
- **Seed Noise Exciter**: Continuous white/pink micro-noise injection to keep self-oscillation sustained without input.

### 6. Non-Linear Saturation & Tube Clamping
- **5 Saturation Curves**: *Warm Tube* (asymmetric 2nd/3rd harmonics), *JFET Scream*, *Diode Clip*, *Hard Clip*, and *Clean Limiter*.
- **Tube Drive**: Variable overdrive for the feedback loop.

### 7. Lookahead Safety Brickwall Limiter
- Internal lookahead brickwall safety limiter and highpass DC blocker guarantees that extreme runaway feedback will never exceed your safety ceiling or blow monitors/headphones.

### 8. Cyber-Acoustic Retina GUI
- **Real-Time FFT Spectrum Visualizer** with live frequency peak lock tracking and input level meter.
- **Acoustic Coupling & Distance Ring Visualizer** pulsating with loop energy.
- **Vacuum Tube Saturation Heat Meter** glowing with filament intensity.
- **Factory Presets**:
  - *Hendrix Strat Bloom*
  - *Cobain Feedback Wall*
  - *PA Vocal Mic Howl (Larsen)*
  - *Sub-Bass Acoustic Drone*
  - *Harmonic 5th Shimmer*
  - *2nd Octave Screamer*
  - *Chaos Ring Mod Squeal*
  - *Pure Feedback Solo (Only Feedback)*

---

## 💾 Pre-Built Downloads

Pre-built releases for macOS (Universal Binary for Apple Silicon `arm64` & Intel `x86_64`) are available on the **[GitHub Releases Page](https://github.com/mbsound/Re-Feedback/releases)**:
- **`Re-Feedback-macOS.dmg`**: Sealed macOS disk image with 1-click installer.
- **`Re-Feedback-macOS.zip`**: Portable zip package.

---

## 🛠️ Building from Source

### macOS (Universal Binary AU / VST3 / Standalone)

**Prerequisites:**
- macOS 10.15 Catalina or newer
- Xcode Command Line Tools (`xcode-select --install`)
- CMake 3.22+ and Ninja (`brew install cmake ninja`)

```bash
# Clone the repository
git clone https://github.com/mbsound/Re-Feedback.git
cd Re-Feedback

# Build using the included build script
./scripts/build_mac.sh
```

The script compiles:
- **Audio Unit (AU v2)** -> `~/Library/Audio/Plug-Ins/Components/Re-Feedback.component`
- **VST3** -> `~/Library/Audio/Plug-Ins/VST3/Re-Feedback.vst3`
- **Standalone App** -> `build_mac/ReFeedback_artefacts/Release/Standalone/Re-Feedback.app`

---

### 🐧 Linux (VST3 / Standalone)

```bash
# Install build dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y libasound2-dev libjack-jackd2-dev ladspa-sdk \
  libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev \
  libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
  libwebkit2gtk-4.0-dev libglu1-mesa-dev mesa-common-dev ninja-build cmake

# Build with CMake
mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja
./DSPTests
```

---

## 🎹 DAW Compatibility

- **Logic Pro, GarageBand**: Uses **Audio Unit (.component)**
- **Ableton Live, FL Studio, Reaper, Bitwig, Cubase, Studio One**: Uses **VST3** or **AU**
- **Standalone**: Run `Re-Feedback.app` directly without a DAW!

In your DAW's plugin browser, look for:
- **Manufacturer / Developer**: `Omega Dumpster`
- **Plugin Name**: `Re-Feedback`

---

## 📜 License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)** — see the [LICENSE](LICENSE) file for details.

---

<p align="center">
  <i>Created with ⚡ by <b>Omega Dumpster</b> (<a href="https://github.com/mbsound">mbsound</a>)</i>
</p>
