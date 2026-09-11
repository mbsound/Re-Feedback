#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace OAF
{

class FractionalDelay
{
public:
    FractionalDelay() = default;

    void prepare(double sampleRate, float maxDelayMs = 200.0f)
    {
        currentSampleRate = sampleRate;
        maxDelaySamples = static_cast<int>(std::ceil(maxDelayMs * 0.001f * currentSampleRate)) + 16;
        bufferSize = 1;
        while (bufferSize < maxDelaySamples * 2)
            bufferSize <<= 1; // Power of 2 for fast wrapping

        bufferMask = bufferSize - 1;
        buffer.assign(bufferSize, 0.0f);
        writeIndex = 0;
        targetDelaySamples = 100.0f;
        currentDelaySamples = 100.0f;
        
        // Hilbert phase rotator all-pass state
        apState1 = 0.0f;
        apState2 = 0.0f;
        phaseRad = 0.0f;
        polarity = 1.0f;
    }

    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
        currentDelaySamples = targetDelaySamples;
        apState1 = 0.0f;
        apState2 = 0.0f;
    }

    void setDelayMs(float delayMs, bool snapImmediate = false)
    {
        float target = (delayMs * 0.001f) * static_cast<float>(currentSampleRate);
        targetDelaySamples = std::clamp(target, 1.0f, static_cast<float>(maxDelaySamples - 8));
        if (snapImmediate)
            currentDelaySamples = targetDelaySamples;
    }

    void setDelaySamples(float delaySamples, bool snapImmediate = false)
    {
        targetDelaySamples = std::clamp(delaySamples, 1.0f, static_cast<float>(maxDelaySamples - 8));
        if (snapImmediate)
            currentDelaySamples = targetDelaySamples;
    }

    void setPhase(float degrees)
    {
        // Convert degrees to radians
        phaseRad = degrees * 3.14159265358979323846f / 180.0f;
    }

    void setPolarityInvert(bool invert)
    {
        polarity = invert ? -1.0f : 1.0f;
    }

    float process(float input)
    {
        // Smooth delay time to avoid pitch jumps during automated sweeps
        currentDelaySamples += (targetDelaySamples - currentDelaySamples) * 0.005f;

        // Write input
        buffer[writeIndex & bufferMask] = input;

        // Hermite 4-point cubic interpolation
        float readPos = static_cast<float>(writeIndex) - currentDelaySamples;
        if (readPos < 0.0f)
            readPos += static_cast<float>(bufferSize);

        int i1 = static_cast<int>(readPos);
        float frac = readPos - static_cast<float>(i1);

        int i0 = (i1 - 1) & bufferMask;
        int i2 = (i1 + 1) & bufferMask;
        int i3 = (i1 + 2) & bufferMask;
        i1 = i1 & bufferMask;

        float y0 = buffer[i0];
        float y1 = buffer[i1];
        float y2 = buffer[i2];
        float y3 = buffer[i3];

        // 4-point, 3rd-order Hermite interpolation
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        float delayedSample = ((c3 * frac + c2) * frac + c1) * frac + c0;

        // Advance write pointer
        writeIndex = (writeIndex + 1) & bufferMask;

        // Apply Phase rotation using 90-degree allpass network
        // y_out = cos(theta) * in + sin(theta) * hilbert(in)
        // First order all-pass approximation of 90-deg phase shift:
        float a1 = 0.6f;
        float apOut = -a1 * delayedSample + apState1;
        apState1 = delayedSample + a1 * apOut;

        float cosPhase = std::cos(phaseRad);
        float sinPhase = std::sin(phaseRad);

        float rotated = (cosPhase * delayedSample + sinPhase * apOut) * polarity;
        return rotated;
    }

private:
    double currentSampleRate = 48000.0;
    int bufferSize = 4096;
    int bufferMask = 4095;
    int maxDelaySamples = 4000;
    std::vector<float> buffer;
    int writeIndex = 0;

    float targetDelaySamples = 100.0f;
    float currentDelaySamples = 100.0f;
    float phaseRad = 0.0f;
    float polarity = 1.0f;

    float apState1 = 0.0f;
    float apState2 = 0.0f;
};

} // namespace OAF
