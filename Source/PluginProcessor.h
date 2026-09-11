#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/FeedbackEngine.h"

namespace OAF
{

class OopsAllFeedbackAudioProcessor : public juce::AudioProcessor
{
public:
    OopsAllFeedbackAudioProcessor();
    ~OopsAllFeedbackAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    FeedbackEngine& getFeedbackEngine() noexcept { return feedbackEngine; }
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Preset definitions
    void loadPreset(int presetIndex);
    static juce::StringArray getPresetNames();

    // Visualizer helper
    std::function<void(const float*, int)> onAudioBlockProcessed;

private:
    void updateParameters();

    FeedbackEngine feedbackEngine;
    juce::AudioProcessorValueTreeState apvts;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OopsAllFeedbackAudioProcessor)
};

} // namespace OAF
