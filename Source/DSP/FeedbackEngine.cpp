#include "FeedbackEngine.h"

namespace OAF
{

FeedbackEngine::FeedbackEngine()
    : rng(1337), noiseDist(-1.0f, 1.0f)
{
}

void FeedbackEngine::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;

    pitchTracker.prepare(sampleRate, maxBlockSize);
    delayL.prepare(sampleRate, 300.0f);
    delayR.prepare(sampleRate, 300.0f);
    safetyLimiter.prepare(sampleRate, 1.5f);

    reset();
}

void FeedbackEngine::reset()
{
    pitchTracker.reset();
    delayL.reset();
    delayR.reset();
    safetyLimiter.reset();

    hpFilterL.reset(); hpFilterR.reset();
    lpFilterL.reset(); lpFilterR.reset();
    peakFilterL.reset(); peakFilterR.reset();
    harmonicCombL.reset(); harmonicCombR.reset();

    feedbackNodeL = 0.0f;
    feedbackNodeR = 0.0f;
    inputEnvelope = 0.0f;
    prevInputSample = 0.0f;
    bloomGain = 0.0f;
    squealBurstEnvelope = 0.0f;
    squealBurstPhase = 0.0f;
    lfoPhase = 0.0f;

    activeFeedbackFreqHz = 440.0f;
    loopEnergy = 0.0f;
    saturationHeat = 0.0f;
    currentInputRms = 0.0f;
    inputCouplingFactor = 0.0f;
    inputFollowerEnvelope = 0.0f;
}

void FeedbackEngine::setParameters(const FeedbackParameters& params)
{
    currentParams = params;

    safetyLimiter.setCeiling(currentParams.outputCeiling);

    // Update Bloom coefficients
    float riseMs = std::max(5.0f, currentParams.bloomRiseMs);
    bloomAttackCoeff = std::exp(-1.0f / (0.001f * riseMs * static_cast<float>(sampleRate)));
    bloomReleaseCoeff = std::exp(-1.0f / (0.001f * 150.0f * static_cast<float>(sampleRate))); // Smooth musical decay

    // Update sculpting filters
    hpFilterL.setHighpass(currentParams.filterLowCutHz, 0.707f, sampleRate);
    hpFilterR.setHighpass(currentParams.filterLowCutHz, 0.707f, sampleRate);

    lpFilterL.setLowpass(currentParams.filterHighCutHz, 0.707f, sampleRate);
    lpFilterR.setLowpass(currentParams.filterHighCutHz, 0.707f, sampleRate);

    peakFilterL.setPeaking(currentParams.peakResonanceHz, currentParams.peakResonanceQ, currentParams.peakResonanceGainDb, sampleRate);
    peakFilterR.setPeaking(currentParams.peakResonanceHz, currentParams.peakResonanceQ, currentParams.peakResonanceGainDb, sampleRate);

    // Delay phase and polarity
    delayL.setPhase(currentParams.phaseDegrees);
    delayR.setPhase(currentParams.phaseDegrees + 12.0f); // Subtle stereo spread
    delayL.setPolarityInvert(currentParams.polarityInvert);
    delayR.setPolarityInvert(currentParams.polarityInvert);
}

void FeedbackEngine::triggerSquealBurst(float intensity)
{
    squealBurstEnvelope = std::clamp(intensity, 0.2f, 1.5f);
    squealBurstFreq = activeFeedbackFreqHz > 50.0f ? activeFeedbackFreqHz : 800.0f;
    squealBurstPhase = 0.0f;
}

float FeedbackEngine::calculateHarmonicFrequency(float basePitchHz, HarmonicInterval interval, float fineCents)
{
    float ratio = 1.0f;
    switch (interval)
    {
        case HarmonicInterval::SubOctave:    ratio = 0.5f; break;
        case HarmonicInterval::Fundamental:  ratio = 1.0f; break;
        case HarmonicInterval::Octave:       ratio = 2.0f; break;
        case HarmonicInterval::Fifth:        ratio = 3.0f; break; // 3rd harmonic = Octave + 5th (standard guitar pickup howl)
        case HarmonicInterval::SecondOctave: ratio = 4.0f; break;
        case HarmonicInterval::MajorThird:   ratio = 5.0f; break;
        case HarmonicInterval::ManualFree:
            return std::clamp(currentParams.manualFreqHz * std::pow(2.0f, fineCents / 1200.0f), 20.0f, 20000.0f);
    }

    float freq = basePitchHz * ratio * std::pow(2.0f, fineCents / 1200.0f);
    return std::clamp(freq, 20.0f, 20000.0f);
}

float FeedbackEngine::applySaturation(float input, SaturationType type, float drive)
{
    float x = input * drive;
    switch (type)
    {
        case SaturationType::Tube:
        {
            // Asymmetric vacuum tube transfer function: rich 2nd and 3rd harmonics
            if (x > 0.0f)
                return std::tanh(x * 0.8f) + 0.15f * std::tanh(x * 2.0f);
            else
                return 0.7f * std::tanh(x * 1.2f);
        }
        case SaturationType::JFET:
        {
            // Spiky transistor scream
            if (x > 1.0f) return 1.0f;
            if (x < -1.0f) return -0.8f;
            return x - 0.25f * (x * x * x);
        }
        case SaturationType::Diode:
        {
            // Germanium diode clipper
            float sign = (x > 0.0f) ? 1.0f : -1.0f;
            float absX = std::abs(x);
            if (absX < 0.3f) return x;
            return sign * (0.3f + 0.7f * (1.0f - std::exp(-(absX - 0.3f) * 1.5f)));
        }
        case SaturationType::HardClip:
        {
            return std::clamp(x, -1.0f, 1.0f);
        }
        case SaturationType::Clean:
        default:
        {
            return std::tanh(x);
        }
    }
}

void FeedbackEngine::processStereo(float* leftChannel, float* rightChannel, int numSamples)
{
    if (sampleRate <= 0.0) return;

    // 1. Process Pitch Tracker and measure input block energy
    float inEnergySum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float inL = leftChannel[i];
        float inR = rightChannel[i];
        inEnergySum += (inL * inL + inR * inR);
        pitchTracker.processSample(0.5f * (inL + inR));
    }

    currentInputRms = std::sqrt(inEnergySum / static_cast<float>(numSamples * 2 + 1e-6f));
    inputFollowerEnvelope = inputFollowerEnvelope * 0.80f + currentInputRms * 0.20f;

    // 2. Determine target feedback frequency
    float detected = pitchTracker.getPitchHz();
    if (currentParams.autoPitchTrack && pitchTracker.isVoiced())
    {
        activeFeedbackFreqHz = calculateHarmonicFrequency(detected, currentParams.harmonic, currentParams.fineTuneCents);
    }
    else if (currentParams.harmonic == HarmonicInterval::ManualFree || !currentParams.autoPitchTrack)
    {
        activeFeedbackFreqHz = calculateHarmonicFrequency(currentParams.manualFreqHz, HarmonicInterval::ManualFree, currentParams.fineTuneCents);
    }

    // 3. Acoustic Loop Delay Time Tuning
    float targetPitch = std::clamp(activeFeedbackFreqHz, 20.0f, 15000.0f);
    float pitchPeriodMs = 1000.0f / targetPitch;

    float distanceDelayMs = currentParams.distanceMs;
    if (currentParams.mode == FeedbackMode::CombResonator)
    {
        distanceDelayMs = pitchPeriodMs;
    }
    else if (currentParams.mode == FeedbackMode::AmpCoupling)
    {
        // In Amp Coupling mode, tune delay to harmonic pitch period multiple + physical distance offset
        int periodMultiplier = std::clamp(static_cast<int>(std::round(currentParams.distanceMs / pitchPeriodMs)), 1, 8);
        distanceDelayMs = static_cast<float>(periodMultiplier) * pitchPeriodMs + (currentParams.distanceMs * 0.04f);
    }
    else if (currentParams.mode == FeedbackMode::ChaosScreamer)
    {
        distanceDelayMs = currentParams.distanceMs * (1.0f + 0.08f * std::sin(lfoPhase));
    }

    delayL.setDelayMs(distanceDelayMs);
    delayR.setDelayMs(distanceDelayMs * 1.015f);

    // 4. Tune harmonic resonator bandpass
    float qHarmonic = (currentParams.mode == FeedbackMode::CombResonator) ? 10.0f 
                    : (currentParams.mode == FeedbackMode::AmpCoupling) ? 4.0f 
                    : 2.0f;
    harmonicCombL.setBandpass(activeFeedbackFreqHz, qHarmonic, sampleRate);
    harmonicCombR.setBandpass(activeFeedbackFreqHz * 1.002f, qHarmonic, sampleRate);

    // 5. Calculate Input Coupling Factor (Prevents runaway squeal on pure silence)
    float coupling = 0.0f;
    if (inputFollowerEnvelope > 0.0006f) // Signal above ~ -64 dBFS
    {
        coupling = std::clamp((inputFollowerEnvelope - 0.0006f) * 25.0f, 0.0f, 1.0f);
    }

    if (currentParams.noiseExciter > 0.005f)
    {
        coupling = std::max(coupling, std::clamp(currentParams.noiseExciter * 2.5f, 0.0f, 1.0f));
    }
    if (squealBurstEnvelope > 0.005f)
    {
        coupling = 1.0f;
    }

    inputCouplingFactor = inputCouplingFactor * 0.85f + coupling * 0.15f;

    // LFO increment for modulation
    float lfoInc = (2.0f * 3.14159265358979323846f * lfoFreq) / static_cast<float>(sampleRate);

    float blockEnergySum = 0.0f;
    float blockHeatSum = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float inL = leftChannel[i];
        float inR = rightChannel[i];
        float inputMag = 0.5f * (std::abs(inL) + std::abs(inR));

        // Transient tracking for dynamic bloom swell
        float delta = inputMag - prevInputSample;
        prevInputSample = inputMag;

        if (delta > 0.05f)
        {
            // Pick transient attack occurred: duck feedback
            float duckFactor = 1.0f - std::clamp(currentParams.duckingAmount, 0.0f, 1.0f);
            bloomGain *= duckFactor;
        }
        else if (inputMag > 0.0008f || currentParams.noiseExciter > 0.005f || squealBurstEnvelope > 0.001f)
        {
            // Note sustained: swell bloom gain
            bloomGain = bloomGain * bloomAttackCoeff + 1.0f * (1.0f - bloomAttackCoeff);
        }
        else
        {
            // Silent: decay bloom gain smoothly
            bloomGain = bloomGain * bloomReleaseCoeff;
        }

        // Exciters
        float exciterSignal = 0.0f;
        if (squealBurstEnvelope > 0.001f)
        {
            squealBurstPhase += (2.0f * 3.14159265358979323846f * squealBurstFreq) / static_cast<float>(sampleRate);
            if (squealBurstPhase > 2.0f * 3.14159265358979323846f)
                squealBurstPhase -= 2.0f * 3.14159265358979323846f;

            squealBurstFreq *= 0.9997f;
            exciterSignal += std::sin(squealBurstPhase) * squealBurstEnvelope * 0.4f;
            squealBurstEnvelope *= 0.9994f;
        }

        if (currentParams.squealTrigger)
        {
            exciterSignal += noiseDist(rng) * 0.3f;
        }

        if (currentParams.noiseExciter > 0.005f)
        {
            exciterSignal += noiseDist(rng) * currentParams.noiseExciter * 0.04f;
        }

        lfoPhase += lfoInc;
        if (lfoPhase > 2.0f * 3.14159265358979323846f)
            lfoPhase -= 2.0f * 3.14159265358979323846f;

        float loopDrive = currentParams.feedbackGain * bloomGain * inputCouplingFactor;

        // If completely uncoupled / silent, decay the past feedback node smoothly to silence
        if (inputCouplingFactor < 0.01f)
        {
            feedbackNodeL *= 0.99f;
            feedbackNodeR *= 0.99f;
        }

        // Feedback Injection: Input + Exciter + Past Feedback
        float feedbackInL = inL + exciterSignal + feedbackNodeL * loopDrive;
        float feedbackInR = inR + exciterSignal + feedbackNodeR * loopDrive;

        if (currentParams.mode == FeedbackMode::ChaosScreamer)
        {
            float modLfo = std::sin(lfoPhase);
            float cross = feedbackInL * 0.25f;
            feedbackInL = (feedbackInL + feedbackInR * 0.3f * modLfo);
            feedbackInR = (feedbackInR + cross * (1.0f - modLfo));
        }

        // Acoustic Distance & Delay Line
        float delayedL = delayL.process(feedbackInL);
        float delayedR = delayR.process(feedbackInR);

        // Sculpting & Resonator Filters
        float filteredL = harmonicCombL.process(delayedL);
        float filteredR = harmonicCombR.process(delayedR);

        filteredL = hpFilterL.process(filteredL);
        filteredR = hpFilterR.process(filteredR);

        filteredL = lpFilterL.process(filteredL);
        filteredR = lpFilterR.process(filteredR);

        filteredL = peakFilterL.process(filteredL);
        filteredR = peakFilterR.process(filteredR);

        // Non-Linear Saturation
        float satL = applySaturation(filteredL, currentParams.saturation, currentParams.drive);
        float satR = applySaturation(filteredR, currentParams.saturation, currentParams.drive);

        feedbackNodeL = satL;
        feedbackNodeR = satR;

        // Wet/Dry Output Routing
        float dryL = currentParams.onlyFeedback ? 0.0f : inL * currentParams.dryGain;
        float dryR = currentParams.onlyFeedback ? 0.0f : inR * currentParams.dryGain;

        float wetL = satL * currentParams.wetGain * 0.75f;
        float wetR = satR * currentParams.wetGain * 0.75f;

        float outL = dryL + wetL;
        float outR = dryR + wetR;

        // Master Safety Limiter
        safetyLimiter.process(outL, outR);

        leftChannel[i] = outL;
        rightChannel[i] = outR;

        blockEnergySum += (satL * satL + satR * satR);
        blockHeatSum += std::abs(satL) + std::abs(satR);
    }

    // Telemetry smoothing
    float currentBlockRms = std::sqrt(blockEnergySum / static_cast<float>(numSamples * 2));
    loopEnergy = loopEnergy * 0.8f + currentBlockRms * 0.2f;

    float currentBlockHeat = std::clamp((blockHeatSum / static_cast<float>(numSamples * 2)) * 0.8f, 0.0f, 1.0f);
    saturationHeat = saturationHeat * 0.85f + currentBlockHeat * 0.15f;
}

} // namespace OAF
