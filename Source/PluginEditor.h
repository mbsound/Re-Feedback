#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GUI/CustomLookAndFeel.h"
#include "GUI/Visualizers.h"

namespace OAF
{

class OopsAllFeedbackAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Button::Listener
{
public:
    explicit OopsAllFeedbackAudioProcessorEditor(OopsAllFeedbackAudioProcessor&);
    ~OopsAllFeedbackAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;

private:
    OopsAllFeedbackAudioProcessor& audioProcessor;
    CustomLookAndFeel customLookAndFeel;

    // Visualizers
    FeedbackSpectrumVisualizer spectrumVisualizer;
    CouplingDistanceVisualizer distanceVisualizer;
    TubeHeatMeter tubeHeatMeter;

    // Header Controls
    juce::Image logoImage;
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::ComboBox presetBox;
    juce::ToggleButton onlyFeedbackToggle { "ONLY FEEDBACK" };

    // Big Squeal Stomp Trigger
    juce::TextButton squealButton { "SQUEAL!" };

    // Selectors
    juce::ComboBox modeBox;
    juce::ComboBox harmonicBox;
    juce::ComboBox saturationBox;
    juce::ToggleButton autoPitchToggle { "Auto Pitch Lock" };
    juce::ToggleButton polarityToggle { "Phase Invert" };

    // Knobs & Labels
    juce::Slider feedbackGainSlider;
    juce::Slider distanceSlider;
    juce::Slider phaseSlider;
    juce::Slider manualFreqSlider;
    juce::Slider fineTuneSlider;

    juce::Slider bloomRiseSlider;
    juce::Slider duckingSlider;
    juce::Slider noiseExciterSlider;

    juce::Slider lowCutSlider;
    juce::Slider highCutSlider;
    juce::Slider peakResonanceHzSlider;
    juce::Slider peakResonanceQSlider;
    juce::Slider peakResonanceGainSlider;

    juce::Slider driveSlider;
    juce::Slider dryGainSlider;
    juce::Slider wetGainSlider;
    juce::Slider outputCeilingSlider;

    // Labels
    std::vector<std::unique_ptr<juce::Label>> sliderLabels;

    // APVTS Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ComboBoxAttachment> modeAttachment;
    std::unique_ptr<ComboBoxAttachment> harmonicAttachment;
    std::unique_ptr<ComboBoxAttachment> saturationAttachment;

    std::unique_ptr<ButtonAttachment> onlyFeedbackAttachment;
    std::unique_ptr<ButtonAttachment> autoPitchAttachment;
    std::unique_ptr<ButtonAttachment> polarityAttachment;
    std::unique_ptr<ButtonAttachment> squealAttachment;

    std::unique_ptr<SliderAttachment> feedbackGainAttachment;
    std::unique_ptr<SliderAttachment> distanceAttachment;
    std::unique_ptr<SliderAttachment> phaseAttachment;
    std::unique_ptr<SliderAttachment> manualFreqAttachment;
    std::unique_ptr<SliderAttachment> fineTuneAttachment;

    std::unique_ptr<SliderAttachment> bloomRiseAttachment;
    std::unique_ptr<SliderAttachment> duckingAttachment;
    std::unique_ptr<SliderAttachment> noiseExciterAttachment;

    std::unique_ptr<SliderAttachment> lowCutAttachment;
    std::unique_ptr<SliderAttachment> highCutAttachment;
    std::unique_ptr<SliderAttachment> peakResonanceHzAttachment;
    std::unique_ptr<SliderAttachment> peakResonanceQAttachment;
    std::unique_ptr<SliderAttachment> peakResonanceGainAttachment;

    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> dryGainAttachment;
    std::unique_ptr<SliderAttachment> wetGainAttachment;
    std::unique_ptr<SliderAttachment> outputCeilingAttachment;

    void setupRotarySlider(juce::Slider& slider, const juce::String& name, const juce::String& suffix = "");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OopsAllFeedbackAudioProcessorEditor)
};

} // namespace OAF
