#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace OAF
{

class SafetyLimiter
{
public:
    SafetyLimiter() = default;

    void prepare(double sampleRate, float lookaheadMs = 1.5f)
    {
        currentSampleRate = sampleRate;
        lookaheadSamples = std::max(1, static_cast<int>(lookaheadMs * 0.001f * currentSampleRate));
        
        delayBufferL.assign(lookaheadSamples + 4, 0.0f);
        delayBufferR.assign(lookaheadSamples + 4, 0.0f);
        delayIndex = 0;

        // Attack is instant (lookahead covers it), Release is smooth (~50ms)
        releaseTimeMs = 50.0f;
        releaseCoeff = std::exp(-1.0f / (0.001f * releaseTimeMs * static_cast<float>(currentSampleRate)));
        
        envelope = 1.0f;
        ceiling = 0.95f; // -0.45 dBFS safety margin
        threshold = 0.85f; // Soft knee threshold

        // DC Blocker state (15 Hz highpass)
        dcX1_L = 0.0f; dcY1_L = 0.0f;
        dcX1_R = 0.0f; dcY1_R = 0.0f;
        dcR = 1.0f - (2.0f * 3.14159265358979323846f * 15.0f / static_cast<float>(currentSampleRate));
    }

    void reset()
    {
        std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
        std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
        delayIndex = 0;
        envelope = 1.0f;
        dcX1_L = dcY1_L = dcX1_R = dcY1_R = 0.0f;
    }

    void setCeiling(float ceilingLinear)
    {
        ceiling = std::clamp(ceilingLinear, 0.1f, 1.0f);
    }

    void process(float& left, float& right)
    {
        // 1. DC Blocker
        float dcL = left - dcX1_L + dcR * dcY1_L;
        dcX1_L = left;
        dcY1_L = dcL;
        left = dcL;

        float dcR_val = right - dcX1_R + dcR * dcY1_R;
        dcX1_R = right;
        dcY1_R = dcR_val;
        right = dcR_val;

        // 2. Peak detector on incoming signal
        float peak = std::max(std::abs(left), std::abs(right));

        // Soft-knee limiting curve calculation
        float targetGain = 1.0f;
        if (peak > threshold)
        {
            float over = peak - threshold;
            float compressed = threshold + (ceiling - threshold) * std::tanh(over / std::max(0.001f, ceiling - threshold));
            targetGain = compressed / peak;
        }

        // Fast attack, smooth release envelope
        if (targetGain < envelope)
        {
            // Attack: instantaneous drop in gain to catch the peak
            envelope = targetGain;
        }
        else
        {
            // Release: smooth exponential return
            envelope = envelope * releaseCoeff + targetGain * (1.0f - releaseCoeff);
        }

        // 3. Store in lookahead delay buffer
        delayBufferL[delayIndex] = left;
        delayBufferR[delayIndex] = right;

        int readIndex = (delayIndex - lookaheadSamples + static_cast<int>(delayBufferL.size())) % delayBufferL.size();
        float delayedL = delayBufferL[readIndex];
        float delayedR = delayBufferR[readIndex];

        delayIndex = (delayIndex + 1) % delayBufferL.size();

        // 4. Apply calculated gain envelope to delayed signal
        left = delayedL * envelope;
        right = delayedR * envelope;

        // Absolute hard safety clamp as final guarantee
        left = std::clamp(left, -ceiling, ceiling);
        right = std::clamp(right, -ceiling, ceiling);
    }

    float getCurrentGainReduction() const noexcept
    {
        return 1.0f - envelope;
    }

private:
    double currentSampleRate = 48000.0;
    int lookaheadSamples = 64;
    int delayIndex = 0;
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;

    float releaseTimeMs = 50.0f;
    float releaseCoeff = 0.999f;
    float envelope = 1.0f;
    float ceiling = 0.95f;
    float threshold = 0.85f;

    // DC Blocker
    float dcX1_L = 0.0f, dcY1_L = 0.0f;
    float dcX1_R = 0.0f, dcY1_R = 0.0f;
    float dcR = 0.998f;
};

} // namespace OAF
