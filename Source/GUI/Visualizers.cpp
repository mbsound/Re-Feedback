#include "Visualizers.h"

namespace OAF
{

// ==============================================================================
// FeedbackSpectrumVisualizer
// ==============================================================================

FeedbackSpectrumVisualizer::FeedbackSpectrumVisualizer(FeedbackEngine& engineRef)
    : engine(engineRef),
      forwardFFT(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    std::fill(fifo.begin(), fifo.end(), 0.0f);
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    std::fill(scopeData.begin(), scopeData.end(), 0.0f);
    startTimerHz(30); // 30 FPS visual update
}

FeedbackSpectrumVisualizer::~FeedbackSpectrumVisualizer()
{
    stopTimer();
}

void FeedbackSpectrumVisualizer::pushAudioBlock(const float* channelData, int numSamples)
{
    if (channelData == nullptr) return;

    for (int i = 0; i < numSamples; ++i)
    {
        if (fifoIndex == fftSize)
        {
            if (!nextFFTBlockReady)
            {
                std::fill(fftData.begin(), fftData.end(), 0.0f);
                std::copy(fifo.begin(), fifo.end(), fftData.begin());
                nextFFTBlockReady = true;
            }
            fifoIndex = 0;
        }

        fifo[fifoIndex++] = channelData[i];
    }
}

void FeedbackSpectrumVisualizer::timerCallback()
{
    if (nextFFTBlockReady)
    {
        window.multiplyWithWindowingTable(fftData.data(), fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

        auto mindB = -80.0f;
        auto maxdB = 10.0f;

        float maxMag = 0.0f;
        int peakBin = 0;

        for (int i = 0; i < 256; ++i)
        {
            auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / 256.0f) * 0.2f);
            auto fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionX * (fftSize / 2)));
            auto level = juce::jmap(juce::jlimit(mindB, maxdB, juce::Decibels::gainToDecibels(fftData[fftDataIndex]) - juce::Decibels::gainToDecibels((float)fftSize)), mindB, maxdB, 0.0f, 1.0f);

            scopeData[i] = scopeData[i] * 0.6f + level * 0.4f;

            if (fftData[fftDataIndex] > maxMag)
            {
                maxMag = fftData[fftDataIndex];
                peakBin = fftDataIndex;
            }
        }

        peakMagnitude = maxMag;
        peakFreqHz = (static_cast<float>(peakBin) * 48000.0f) / static_cast<float>(fftSize);
        nextFFTBlockReady = false;
        repaint();
    }
    else
    {
        // Decay slightly if no new blocks
        for (auto& s : scopeData)
            s *= 0.9f;
        repaint();
    }
}

void FeedbackSpectrumVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background panel
    g.setColour(juce::Colour(0xff0d0f13));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff222834));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // Grid lines
    g.setColour(juce::Colour(0x1a00d4ff));
    for (float f : { 100.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f })
    {
        float normX = std::log10(f / 20.0f) / std::log10(20000.0f / 20.0f);
        float x = bounds.getX() + normX * bounds.getWidth();
        g.drawVerticalLine(static_cast<int>(x), bounds.getY() + 4.0f, bounds.getBottom() - 4.0f);
    }

    // Spectrum Curve Path
    juce::Path spectrumPath;
    spectrumPath.startNewSubPath(bounds.getX(), bounds.getBottom());

    for (size_t i = 0; i < scopeData.size(); ++i)
    {
        float x = juce::jmap((float)i, 0.0f, (float)scopeData.size(), bounds.getX(), bounds.getRight());
        float y = juce::jmap(scopeData[i], 0.0f, 1.0f, bounds.getBottom() - 2.0f, bounds.getY() + 10.0f);
        spectrumPath.lineTo(x, y);
    }
    spectrumPath.lineTo(bounds.getRight(), bounds.getBottom());
    spectrumPath.closeSubPath();

    // Gradient Fill under spectrum
    juce::ColourGradient grad(juce::Colour(0xaaff5500), bounds.getCentreX(), bounds.getY(),
                              juce::Colour(0x1100d4ff), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill(grad);
    g.fillPath(spectrumPath);

    // Glowing Spectrum Top Outline
    juce::Path outlinePath;
    for (size_t i = 0; i < scopeData.size(); ++i)
    {
        float x = juce::jmap((float)i, 0.0f, (float)scopeData.size(), bounds.getX(), bounds.getRight());
        float y = juce::jmap(scopeData[i], 0.0f, 1.0f, bounds.getBottom() - 2.0f, bounds.getY() + 10.0f);
        if (i == 0) outlinePath.startNewSubPath(x, y);
        else outlinePath.lineTo(x, y);
    }
    g.setColour(juce::Colour(0xffff9900));
    g.strokePath(outlinePath, juce::PathStrokeType(2.0f));

    // Active Feedback Peak Indicator
    float targetHz = engine.getTargetFeedbackHz();
    float normTarget = std::clamp(std::log10(targetHz / 20.0f) / std::log10(20000.0f / 20.0f), 0.0f, 1.0f);
    float targetX = bounds.getX() + normTarget * bounds.getWidth();

    // Glowing vertical resonance spike
    g.setColour(juce::Colour(0x6600f0ff));
    g.drawVerticalLine(static_cast<int>(targetX), bounds.getY() + 2.0f, bounds.getBottom() - 2.0f);
    g.setColour(juce::Colour(0xff00f0ff));
    g.drawVerticalLine(static_cast<int>(targetX), bounds.getY() + 8.0f, bounds.getBottom() - 8.0f);

    // Frequency readout tag
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    juce::String tag = juce::String::formatted("LOCK: %.1f Hz", targetHz);
    if (engine.isPitchVoiced())
        tag += juce::String::formatted(" (PITCH: %.1f Hz)", engine.getDetectedPitchHz());

    g.setColour(juce::Colour(0xff00f0ff));
    g.drawText(tag, bounds.getX() + 8, bounds.getY() + 4, 250, 16, juce::Justification::left);

    // Live Input Audio Activity / Mic Indicator
    float inRms = engine.getInputRms();
    float inDb = inRms > 0.00001f ? juce::Decibels::gainToDecibels(inRms) : -100.0f;
    if (inDb > -55.0f)
    {
        g.setColour(juce::Colour(0xff00ff88));
        g.fillEllipse(bounds.getRight() - 110.0f, bounds.getY() + 7.0f, 8.0f, 8.0f);
        g.drawText(juce::String::formatted("INPUT: %.1f dB", inDb), bounds.getRight() - 98.0f, bounds.getY() + 4.0f, 90.0f, 16.0f, juce::Justification::left);
    }
    else
    {
        g.setColour(juce::Colour(0xff667788));
        g.drawEllipse(bounds.getRight() - 110.0f, bounds.getY() + 7.0f, 8.0f, 8.0f, 1.0f);
        g.drawText("INPUT: IDLE", bounds.getRight() - 98.0f, bounds.getY() + 4.0f, 90.0f, 16.0f, juce::Justification::left);
    }
}

// ==============================================================================
// CouplingDistanceVisualizer
// ==============================================================================

CouplingDistanceVisualizer::CouplingDistanceVisualizer(FeedbackEngine& engineRef)
    : engine(engineRef)
{
    startTimerHz(30);
}

CouplingDistanceVisualizer::~CouplingDistanceVisualizer()
{
    stopTimer();
}

void CouplingDistanceVisualizer::timerCallback()
{
    float energy = engine.getLoopEnergy();
    animationPhase += 0.05f + energy * 0.2f;
    if (animationPhase > 2.0f * 3.14159265358979323846f)
        animationPhase -= 2.0f * 3.14159265358979323846f;
    repaint();
}

void CouplingDistanceVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto centre = bounds.getCentre();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.44f;

    // Background circle
    g.setColour(juce::Colour(0xff0e1015));
    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    g.setColour(juce::Colour(0xff222834));
    g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.2f);

    // Pulsating Acoustic Feedback Waves
    float energy = engine.getLoopEnergy();
    int numRings = 4;
    for (int i = 0; i < numRings; ++i)
    {
        float ringProgress = std::fmod(animationPhase + (float)i * (2.0f * 3.14159265358979323846f / numRings), 2.0f * 3.14159265358979323846f) / (2.0f * 3.14159265358979323846f);
        float currentR = radius * ringProgress;
        float alpha = (1.0f - ringProgress) * (0.2f + energy * 0.8f);

        juce::Colour waveCol = juce::Colour(0xffff5500).withAlpha(std::clamp(alpha, 0.0f, 1.0f));
        g.setColour(waveCol);
        g.drawEllipse(centre.x - currentR, centre.y - currentR, currentR * 2.0f, currentR * 2.0f, 1.5f + energy * 2.0f);
    }

    // Center Emitter Node (Microphone / Guitar Pickup)
    float emitterRadius = 6.0f + energy * 4.0f;
    g.setColour(juce::Colour(0xff00d4ff));
    g.fillEllipse(centre.x - emitterRadius, centre.y - emitterRadius, emitterRadius * 2.0f, emitterRadius * 2.0f);

    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff8b949e));
    g.drawText("COUPLING", bounds.getX(), bounds.getBottom() - 14, bounds.getWidth(), 12, juce::Justification::centred);
}

// ==============================================================================
// TubeHeatMeter
// ==============================================================================

TubeHeatMeter::TubeHeatMeter(FeedbackEngine& engineRef)
    : engine(engineRef)
{
    startTimerHz(30);
}

TubeHeatMeter::~TubeHeatMeter()
{
    stopTimer();
}

void TubeHeatMeter::timerCallback()
{
    float heat = engine.getSaturationHeat();
    smoothedHeat = smoothedHeat * 0.8f + heat * 0.2f;
    repaint();
}

void TubeHeatMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background tube capsule
    g.setColour(juce::Colour(0xff0a0c10));
    g.fillRoundedRectangle(bounds, bounds.getWidth() * 0.5f);
    g.setColour(juce::Colour(0xff2b3140));
    g.drawRoundedRectangle(bounds, bounds.getWidth() * 0.5f, 1.2f);

    // Glowing Filament Indicator
    auto innerBounds = bounds.reduced(3.0f);
    float fillHeight = innerBounds.getHeight() * std::clamp(smoothedHeat, 0.05f, 1.0f);
    auto heatRect = juce::Rectangle<float>(innerBounds.getX(), innerBounds.getBottom() - fillHeight, innerBounds.getWidth(), fillHeight);

    juce::ColourGradient heatGrad(juce::Colour(0xffff2200), innerBounds.getCentreX(), innerBounds.getY(),
                                  juce::Colour(0xffff9900), innerBounds.getCentreX(), innerBounds.getBottom(), false);
    g.setGradientFill(heatGrad);
    g.fillRoundedRectangle(heatRect, bounds.getWidth() * 0.4f);

    // Filament grid lines
    g.setColour(juce::Colour(0x44ffffff));
    for (float y = innerBounds.getY() + 4.0f; y < innerBounds.getBottom() - 4.0f; y += 6.0f)
    {
        g.drawHorizontalLine(static_cast<int>(y), innerBounds.getX() + 2.0f, innerBounds.getRight() - 2.0f);
    }
}

} // namespace OAF
