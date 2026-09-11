#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace OAF
{

class PitchTracker
{
public:
    PitchTracker() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;
        
        // Window size for pitch detection: 2048 samples (~46ms at 44.1kHz, gives accurate detection down to ~45Hz)
        bufferSize = 2048;
        audioBuffer.assign(bufferSize, 0.0f);
        nsdfBuffer.assign(bufferSize, 0.0f);
        writeIndex = 0;
        
        // Range: 50 Hz (low guitar E2 is 82.4Hz, bass drop D is 73.4Hz) to 1500 Hz
        minPeriod = static_cast<int>(currentSampleRate / 1500.0);
        maxPeriod = static_cast<int>(currentSampleRate / 50.0);
        if (maxPeriod >= bufferSize / 2)
            maxPeriod = bufferSize / 2 - 1;
        if (minPeriod < 2)
            minPeriod = 2;

        currentPitchHz = 220.0f; // Default A3
        smoothedPitchHz = 220.0f;
        confidence = 0.0f;
        inputRms = 0.0f;
        sampleCounter = 0;
        analysisHop = 128; // Run analysis every 128 samples
    }

    void processSample(float sample)
    {
        audioBuffer[writeIndex] = sample;
        writeIndex = (writeIndex + 1) % bufferSize;

        sampleCounter++;
        if (sampleCounter >= analysisHop)
        {
            sampleCounter = 0;
            detectPitch();
        }
    }

    void processBlock(const float* input, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            processSample(input[i]);
        }
    }

    float getPitchHz() const noexcept { return smoothedPitchHz; }
    float getConfidence() const noexcept { return confidence; }
    float getInputRms() const noexcept { return inputRms; }
    bool isVoiced() const noexcept { return confidence > 0.40f && inputRms > 0.001f; }

    void reset()
    {
        std::fill(audioBuffer.begin(), audioBuffer.end(), 0.0f);
        std::fill(nsdfBuffer.begin(), nsdfBuffer.end(), 0.0f);
        writeIndex = 0;
        currentPitchHz = 220.0f;
        smoothedPitchHz = 220.0f;
        confidence = 0.0f;
        inputRms = 0.0f;
        sampleCounter = 0;
    }

private:
    void detectPitch()
    {
        if (currentSampleRate <= 0.0) return;

        // 1. Compute RMS energy of buffer
        float energy = 0.0f;
        for (int i = 0; i < bufferSize; ++i)
        {
            energy += audioBuffer[i] * audioBuffer[i];
        }
        
        inputRms = std::sqrt(energy / static_cast<float>(bufferSize));
        
        if (inputRms < 0.0005f) // Signal is silence / noise floor (-66 dBFS)
        {
            confidence *= 0.8f;
            return;
        }

        // 2. Compute Normalized Square Difference Function (NSDF)
        int tauMax = maxPeriod;
        int tauMin = minPeriod;

        for (int tau = 0; tau <= tauMax; ++tau)
        {
            float acf = 0.0f;
            float divisor = 0.0f;

            for (int i = 0; i < bufferSize - tau; ++i)
            {
                int idx1 = (writeIndex + i) % bufferSize;
                int idx2 = (writeIndex + i + tau) % bufferSize;
                float x1 = audioBuffer[idx1];
                float x2 = audioBuffer[idx2];
                acf += x1 * x2;
                divisor += x1 * x1 + x2 * x2;
            }

            if (divisor > 0.000001f)
                nsdfBuffer[tau] = 2.0f * acf / divisor;
            else
                nsdfBuffer[tau] = 0.0f;
        }

        // 3. Find highest NSDF peak (McLeod Pitch Method)
        float maxVal = -1.0f;
        for (int tau = tauMin; tau < tauMax - 1; ++tau)
        {
            if (nsdfBuffer[tau] > nsdfBuffer[tau - 1] && nsdfBuffer[tau] >= nsdfBuffer[tau + 1])
            {
                if (nsdfBuffer[tau] > maxVal)
                    maxVal = nsdfBuffer[tau];
            }
        }

        if (maxVal < 0.35f)
        {
            confidence *= 0.85f;
            return;
        }

        // Find the FIRST peak that reaches 80% of maxVal to pick fundamental over octaves
        float threshold = maxVal * 0.80f;
        int bestTau = -1;
        float bestVal = 0.0f;

        for (int tau = tauMin; tau < tauMax - 1; ++tau)
        {
            if (nsdfBuffer[tau] > nsdfBuffer[tau - 1] && nsdfBuffer[tau] >= nsdfBuffer[tau + 1])
            {
                if (nsdfBuffer[tau] >= threshold)
                {
                    bestTau = tau;
                    bestVal = nsdfBuffer[tau];
                    break;
                }
            }
        }

        if (bestTau > 0)
        {
            // Parabolic interpolation for sub-sample accuracy
            float y0 = nsdfBuffer[bestTau - 1];
            float y1 = nsdfBuffer[bestTau];
            float y2 = nsdfBuffer[bestTau + 1];
            float denom = 2.0f * (2.0f * y1 - y0 - y2);
            float delta = 0.0f;
            if (std::abs(denom) > 1e-6f)
            {
                delta = (y0 - y2) / denom;
            }

            float refinedTau = static_cast<float>(bestTau) + delta;
            if (refinedTau > 1.0f)
            {
                float rawPitch = static_cast<float>(currentSampleRate) / refinedTau;
                if (rawPitch >= 40.0f && rawPitch <= 2000.0f)
                {
                    currentPitchHz = rawPitch;
                    confidence = bestVal;

                    // Smooth pitch transition to prevent glitches
                    float smoothWeight = 0.35f;
                    if (std::abs(smoothedPitchHz - currentPitchHz) > 80.0f)
                        smoothWeight = 0.7f; // Faster lock on note changes

                    smoothedPitchHz = smoothedPitchHz * (1.0f - smoothWeight) + currentPitchHz * smoothWeight;
                    return;
                }
            }
        }

        confidence *= 0.85f;
    }

    double currentSampleRate = 48000.0;
    int bufferSize = 2048;
    std::vector<float> audioBuffer;
    std::vector<float> nsdfBuffer;
    int writeIndex = 0;
    int minPeriod = 32;
    int maxPeriod = 960;
    int sampleCounter = 0;
    int analysisHop = 128;

    float currentPitchHz = 220.0f;
    float smoothedPitchHz = 220.0f;
    float confidence = 0.0f;
    float inputRms = 0.0f;
};

} // namespace OAF
