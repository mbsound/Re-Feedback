#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "../DSP/FeedbackEngine.h"

namespace OAF
{

class FeedbackSpectrumVisualizer : public juce::Component, public juce::Timer
{
public:
    FeedbackSpectrumVisualizer(FeedbackEngine& engineRef);
    ~FeedbackSpectrumVisualizer() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;
    void pushAudioBlock(const float* channelData, int numSamples);

private:
    FeedbackEngine& engine;

    static constexpr auto fftOrder = 10;
    static constexpr auto fftSize = 1 << fftOrder;
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, fftSize> fifo;
    std::array<float, fftSize * 2> fftData;
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

    std::array<float, 256> scopeData;
    float peakFreqHz = 440.0f;
    float peakMagnitude = 0.0f;
};

class CouplingDistanceVisualizer : public juce::Component, public juce::Timer
{
public:
    CouplingDistanceVisualizer(FeedbackEngine& engineRef);
    ~CouplingDistanceVisualizer() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    FeedbackEngine& engine;
    float animationPhase = 0.0f;
};

class TubeHeatMeter : public juce::Component, public juce::Timer
{
public:
    TubeHeatMeter(FeedbackEngine& engineRef);
    ~TubeHeatMeter() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    FeedbackEngine& engine;
    float smoothedHeat = 0.0f;
};

} // namespace OAF
