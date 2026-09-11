#include "../Source/DSP/PitchTracker.h"
#include "../Source/DSP/FractionalDelay.h"
#include "../Source/DSP/SafetyLimiter.h"
#include "../Source/DSP/FeedbackEngine.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

void testPitchTracker()
{
    std::cout << "[TEST] Running PitchTracker verification...\n";
    OAF::PitchTracker tracker;
    double sampleRate = 48000.0;
    tracker.prepare(sampleRate, 512);

    // Generate a 440 Hz sine wave (A4)
    float testFreq = 440.0f;
    std::vector<float> sineWave(48000);
    for (size_t i = 0; i < sineWave.size(); ++i)
    {
        sineWave[i] = std::sin(2.0f * 3.14159265358979323846f * testFreq * static_cast<float>(i) / static_cast<float>(sampleRate));
    }

    // Feed through tracker
    for (size_t i = 0; i < sineWave.size(); i += 128)
    {
        tracker.processBlock(&sineWave[i], 128);
    }

    float detected = tracker.getPitchHz();
    float confidence = tracker.getConfidence();
    std::cout << "  - Generated 440.0 Hz -> Detected: " << detected << " Hz (Confidence: " << confidence << ")\n";

    assert(std::abs(detected - 440.0f) < 8.0f);
    assert(tracker.isVoiced());
    std::cout << "  [PASS] PitchTracker test passed!\n";
}

void testFractionalDelay()
{
    std::cout << "[TEST] Running FractionalDelay verification...\n";
    OAF::FractionalDelay delay;
    double sampleRate = 48000.0;
    delay.prepare(sampleRate, 100.0f);
    delay.setDelayMs(10.0f, true); // 10ms = 480 samples, snap immediately for test

    // Feed an impulse and verify delay
    std::vector<float> output(1000, 0.0f);
    for (int i = 0; i < 1000; ++i)
    {
        float in = (i == 0) ? 1.0f : 0.0f;
        output[i] = delay.process(in);
    }

    // Find peak index
    int peakIndex = 0;
    float peakVal = 0.0f;
    for (int i = 0; i < 1000; ++i)
    {
        if (std::abs(output[i]) > peakVal)
        {
            peakVal = std::abs(output[i]);
            peakIndex = i;
        }
    }

    std::cout << "  - 10.0ms delay at 48kHz (expected ~480 samples) -> Peak at sample " << peakIndex << "\n";
    assert(peakIndex >= 475 && peakIndex <= 485);
    std::cout << "  [PASS] FractionalDelay test passed!\n";
}

void testSafetyLimiter()
{
    std::cout << "[TEST] Running SafetyLimiter brickwall ceiling test...\n";
    OAF::SafetyLimiter limiter;
    limiter.prepare(48000.0, 1.5f);
    limiter.setCeiling(0.95f);

    // Feed extreme runaway signal (+30 dBFS peak = 31.6 linear)
    float maxOut = 0.0f;
    for (int i = 0; i < 2000; ++i)
    {
        float l = std::sin(static_cast<float>(i) * 0.1f) * 30.0f;
        float r = std::cos(static_cast<float>(i) * 0.1f) * 30.0f;
        limiter.process(l, r);
        maxOut = std::max(maxOut, std::max(std::abs(l), std::abs(r)));
    }

    std::cout << "  - Input magnitude 30.0 (+30dB) -> Output ceiling max: " << maxOut << " (Limit 0.95)\n";
    assert(maxOut <= 0.9501f);
    std::cout << "  [PASS] SafetyLimiter brickwall ceiling passed!\n";
}

void testFeedbackEngine()
{
    std::cout << "[TEST] Running FeedbackEngine full loop verification...\n";
    OAF::FeedbackEngine engine;
    engine.prepare(48000.0, 256);

    OAF::FeedbackParameters params;
    params.mode = OAF::FeedbackMode::AmpCoupling;
    params.harmonic = OAF::HarmonicInterval::Octave;
    params.feedbackGain = 1.8f; // High gain for self-oscillation
    params.bloomRiseMs = 50.0f; // Fast bloom for test
    params.noiseExciter = 0.05f; // Seed noise
    engine.setParameters(params);

    std::vector<float> left(48000, 0.0f);
    std::vector<float> right(48000, 0.0f);

    // Strike an initial note (A3 = 220Hz)
    for (int i = 0; i < 4800; ++i)
    {
        float note = std::sin(2.0f * 3.14159265358979323846f * 220.0f * static_cast<float>(i) / 48000.0f);
        left[i] = note * 0.5f;
        right[i] = note * 0.5f;
    }

    // Process blocks
    for (int i = 0; i < 48000; i += 256)
    {
        int blockSize = std::min(256, 48000 - i);
        engine.processStereo(&left[i], &right[i], blockSize);
    }

    float loopEnergy = engine.getLoopEnergy();
    float saturationHeat = engine.getSaturationHeat();
    std::cout << "  - Feedback Loop Energy: " << loopEnergy << ", Saturation Heat: " << saturationHeat << "\n";

    // Verify feedback maintained energy without exploding or producing NaN/Inf
    assert(loopEnergy > 0.01f);
    for (int i = 0; i < 48000; ++i)
    {
        assert(!std::isnan(left[i]) && !std::isinf(left[i]));
        assert(!std::isnan(right[i]) && !std::isinf(right[i]));
        assert(std::abs(left[i]) <= 1.0f);
        assert(std::abs(right[i]) <= 1.0f);
    }

    std::cout << "  [PASS] FeedbackEngine full loop verification passed!\n";
}

void testSilenceStability()
{
    std::cout << "[TEST] Running Silence Stability & Gating test (No runaway on unplugged mic)...\n";
    OAF::FeedbackEngine engine;
    engine.prepare(48000.0, 256);

    OAF::FeedbackParameters params;
    params.mode = OAF::FeedbackMode::AmpCoupling;
    params.harmonic = OAF::HarmonicInterval::Octave;
    params.feedbackGain = 1.8f; // High feedback gain
    params.noiseExciter = 0.0f; // Default clean setup
    engine.setParameters(params);

    // Feed 2 seconds of pure silence (all 0.0f)
    std::vector<float> left(48000 * 2, 0.0f);
    std::vector<float> right(48000 * 2, 0.0f);

    for (size_t i = 0; i < left.size(); i += 256)
    {
        engine.processStereo(&left[i], &right[i], 256);
    }

    float maxSilentOut = 0.0f;
    for (size_t i = 0; i < left.size(); ++i)
    {
        maxSilentOut = std::max(maxSilentOut, std::max(std::abs(left[i]), std::abs(right[i])));
    }

    float loopEnergy = engine.getLoopEnergy();
    std::cout << "  - Max output on silence: " << maxSilentOut << ", Loop Energy: " << loopEnergy << "\n";
    assert(maxSilentOut < 0.0001f);
    assert(loopEnergy < 0.001f);
    std::cout << "  [PASS] Silence Stability verified! No screaming or runaway on silent/unplugged input.\n";
}

int main()
{
    std::cout << "========================================\n";
    std::cout << "   Oops All Feedback - DSP Test Suite   \n";
    std::cout << "========================================\n";

    testPitchTracker();
    testFractionalDelay();
    testSafetyLimiter();
    testFeedbackEngine();
    testSilenceStability();

    std::cout << "\n>>> ALL DSP TESTS PASSED SUCCESSFULLY! <<<\n";
    return 0;
}
