#pragma once

#include "PitchTracker.h"
#include "FractionalDelay.h"
#include "SafetyLimiter.h"
#include <cmath>
#include <vector>
#include <random>

namespace OAF
{

enum class FeedbackMode
{
    AmpCoupling = 0,
    LarsenHowl,
    CombResonator,
    ChaosScreamer
};

enum class HarmonicInterval
{
    SubOctave = 0,    // 0.5x (-12 st)
    Fundamental,      // 1.0x (Unison)
    Octave,           // 2.0x (+12 st)
    Fifth,            // 3.0x (+19 st)
    SecondOctave,     // 4.0x (+24 st)
    MajorThird,       // 5.0x (+28 st)
    ManualFree        // Continuous manual frequency
};

enum class SaturationType
{
    Tube = 0,
    JFET,
    Diode,
    HardClip,
    Clean
};

struct FeedbackParameters
{
    FeedbackMode mode = FeedbackMode::AmpCoupling;
    HarmonicInterval harmonic = HarmonicInterval::Octave;
    SaturationType saturation = SaturationType::Tube;

    float feedbackGain = 1.2f;        // 0.0 to 3.0 (can drive into intense runaway)
    float distanceMs = 5.0f;          // 0.1ms to 100ms
    float phaseDegrees = 0.0f;        // 0 to 360 deg
    bool polarityInvert = false;      // Phase 180 flip

    bool autoPitchTrack = true;       // Track incoming pitch automatically
    float manualFreqHz = 440.0f;      // 20 Hz to 20 kHz
    float fineTuneCents = 0.0f;       // -100 to +100 cents

    float filterLowCutHz = 80.0f;     // Highpass
    float filterHighCutHz = 8000.0f;  // Lowpass
    float peakResonanceHz = 1500.0f;  // Resonant howl center freq
    float peakResonanceQ = 4.0f;      // Q up to 40 for screaming peaks
    float peakResonanceGainDb = 6.0f; // Boost in feedback path

    float bloomRiseMs = 350.0f;       // Dynamic feedback swell time
    float duckingAmount = 0.5f;       // Duck feedback during pick transient
    float noiseExciter = 0.02f;       // Seed noise level to jumpstart feedback
    bool squealTrigger = false;       // Momentary burst injection

    bool onlyFeedback = false;        // Strip dry signal completely
    float dryGain = 1.0f;             // 0.0 to 1.0
    float wetGain = 1.0f;             // 0.0 to 2.0
    float drive = 2.0f;               // Saturation drive
    float outputCeiling = 0.95f;      // Safety Limiter ceiling
};

class BiquadFilter
{
public:
    BiquadFilter() = default;

    void reset()
    {
        x1 = x2 = y1 = y2 = 0.0f;
    }

    void setHighpass(float freq, float q, double sampleRate)
    {
        if (freq <= 10.0f || sampleRate <= 0.0) return;
        float w0 = 2.0f * 3.14159265358979323846f * freq / static_cast<float>(sampleRate);
        w0 = std::clamp(w0, 0.001f, 3.1f);
        float alpha = std::sin(w0) / (2.0f * std::max(0.1f, q));
        float cosw0 = std::cos(w0);

        float b0 = (1.0f + cosw0) * 0.5f;
        float b1 = -(1.0f + cosw0);
        float b2 = (1.0f + cosw0) * 0.5f;
        float a0 = 1.0f + alpha;
        float a1 = -2.0f * cosw0;
        float a2 = 1.0f - alpha;

        setCoeffs(b0/a0, b1/a0, b2/a0, a1/a0, a2/a0);
    }

    void setLowpass(float freq, float q, double sampleRate)
    {
        if (freq <= 10.0f || sampleRate <= 0.0) return;
        float w0 = 2.0f * 3.14159265358979323846f * freq / static_cast<float>(sampleRate);
        w0 = std::clamp(w0, 0.001f, 3.1f);
        float alpha = std::sin(w0) / (2.0f * std::max(0.1f, q));
        float cosw0 = std::cos(w0);

        float b0 = (1.0f - cosw0) * 0.5f;
        float b1 = 1.0f - cosw0;
        float b2 = (1.0f - cosw0) * 0.5f;
        float a0 = 1.0f + alpha;
        float a1 = -2.0f * cosw0;
        float a2 = 1.0f - alpha;

        setCoeffs(b0/a0, b1/a0, b2/a0, a1/a0, a2/a0);
    }

    void setPeaking(float freq, float q, float gainDb, double sampleRate)
    {
        if (freq <= 10.0f || sampleRate <= 0.0) return;
        float w0 = 2.0f * 3.14159265358979323846f * freq / static_cast<float>(sampleRate);
        w0 = std::clamp(w0, 0.001f, 3.1f);
        float A = std::pow(10.0f, gainDb / 40.0f);
        float alpha = std::sin(w0) / (2.0f * std::max(0.1f, q));
        float cosw0 = std::cos(w0);

        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * cosw0;
        float b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * cosw0;
        float a2 = 1.0f - alpha / A;

        setCoeffs(b0/a0, b1/a0, b2/a0, a1/a0, a2/a0);
    }

    void setBandpass(float freq, float q, double sampleRate)
    {
        if (freq <= 10.0f || sampleRate <= 0.0) return;
        float w0 = 2.0f * 3.14159265358979323846f * freq / static_cast<float>(sampleRate);
        w0 = std::clamp(w0, 0.001f, 3.1f);
        float alpha = std::sin(w0) / (2.0f * std::max(0.1f, q));
        float cosw0 = std::cos(w0);

        float b0 = alpha;
        float b1 = 0.0f;
        float b2 = -alpha;
        float a0 = 1.0f + alpha;
        float a1 = -2.0f * cosw0;
        float a2 = 1.0f - alpha;

        setCoeffs(b0/a0, b1/a0, b2/a0, a1/a0, a2/a0);
    }

    inline float process(float input)
    {
        float out = c_b0 * input + c_b1 * x1 + c_b2 * x2 - c_a1 * y1 - c_a2 * y2;
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = out;
        return out;
    }

private:
    void setCoeffs(float b0, float b1, float b2, float a1, float a2)
    {
        c_b0 = b0; c_b1 = b1; c_b2 = b2;
        c_a1 = a1; c_a2 = a2;
    }

    float c_b0 = 1.0f, c_b1 = 0.0f, c_b2 = 0.0f;
    float c_a1 = 0.0f, c_a2 = 0.0f;
    float x1 = 0.0f, x2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;
};

class FeedbackEngine
{
public:
    FeedbackEngine();
    ~FeedbackEngine() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setParameters(const FeedbackParameters& params);
    const FeedbackParameters& getParameters() const noexcept { return currentParams; }

    void triggerSquealBurst(float intensity = 1.0f);

    void processStereo(float* leftChannel, float* rightChannel, int numSamples);

    // Telemetry for GUI visualization
    float getDetectedPitchHz() const noexcept { return pitchTracker.getPitchHz(); }
    float getTargetFeedbackHz() const noexcept { return activeFeedbackFreqHz; }
    float getLoopEnergy() const noexcept { return loopEnergy; }
    float getSaturationHeat() const noexcept { return saturationHeat; }
    float getGainReduction() const noexcept { return safetyLimiter.getCurrentGainReduction(); }
    bool isPitchVoiced() const noexcept { return pitchTracker.isVoiced(); }
    float getInputRms() const noexcept { return currentInputRms; }
    float getInputActivity() const noexcept { return inputCouplingFactor; }

private:
    float applySaturation(float input, SaturationType type, float drive);
    float calculateHarmonicFrequency(float basePitchHz, HarmonicInterval interval, float fineCents);

    double sampleRate = 48000.0;
    FeedbackParameters currentParams;

    PitchTracker pitchTracker;
    FractionalDelay delayL;
    FractionalDelay delayR;
    SafetyLimiter safetyLimiter;

    // Sculpting Filters for Left and Right feedback loop paths
    BiquadFilter hpFilterL, hpFilterR;
    BiquadFilter lpFilterL, lpFilterR;
    BiquadFilter peakFilterL, peakFilterR;
    BiquadFilter harmonicCombL, harmonicCombR;

    // Feedback memory nodes
    float feedbackNodeL = 0.0f;
    float feedbackNodeR = 0.0f;

    // Transient detector & Dynamic Bloom Swell
    float inputEnvelope = 0.0f;
    float prevInputSample = 0.0f;
    float bloomGain = 0.0f;
    float bloomAttackCoeff = 0.999f;
    float bloomReleaseCoeff = 0.999f;

    // Squeal Trigger Generator
    float squealBurstEnvelope = 0.0f;
    float squealBurstPhase = 0.0f;
    float squealBurstFreq = 800.0f;

    // Mod LFO for Chaos / Flutter
    float lfoPhase = 0.0f;
    float lfoFreq = 2.5f;

    // PRNG for Seed Noise Exciter
    std::mt19937 rng;
    std::uniform_real_distribution<float> noiseDist;

    // Telemetry state
    float activeFeedbackFreqHz = 440.0f;
    float loopEnergy = 0.0f;
    float saturationHeat = 0.0f;
    float currentInputRms = 0.0f;
    float inputCouplingFactor = 0.0f;
    float inputFollowerEnvelope = 0.0f;
};

} // namespace OAF
