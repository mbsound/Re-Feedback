#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>

namespace OAF
{

OopsAllFeedbackAudioProcessorEditor::OopsAllFeedbackAudioProcessorEditor(OopsAllFeedbackAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      spectrumVisualizer(p.getFeedbackEngine()),
      distanceVisualizer(p.getFeedbackEngine()),
      tubeHeatMeter(p.getFeedbackEngine())
{
    setLookAndFeel(&customLookAndFeel);

    // Load Oops All Feedback logo image
    logoImage = juce::ImageFileFormat::loadFrom(BinaryData::icon_png, (size_t) BinaryData::icon_pngSize);

    // Audio block hook for spectrum visualizer
    audioProcessor.onAudioBlockProcessed = [this](const float* data, int numSamples) {
        spectrumVisualizer.pushAudioBlock(data, numSamples);
    };

    // Header Title
    titleLabel.setText("RE-FEEDBACK", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff6600));
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("Pure Feedback Synthesizer - Created by Omega Dumpster", juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(11.0f, juce::Font::italic));
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8a93a5));
    addAndMakeVisible(subtitleLabel);

    // Preset Dropdown
    auto presets = OopsAllFeedbackAudioProcessor::getPresetNames();
    for (int i = 0; i < presets.size(); ++i)
        presetBox.addItem(presets[i], i + 1);
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.onChange = [this]() {
        audioProcessor.loadPreset(presetBox.getSelectedId() - 1);
    };
    addAndMakeVisible(presetBox);

    // ONLY FEEDBACK Toggle
    onlyFeedbackToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffff3300));
    onlyFeedbackToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff5500));
    addAndMakeVisible(onlyFeedbackToggle);

    // Visualizer Components
    addAndMakeVisible(spectrumVisualizer);
    addAndMakeVisible(distanceVisualizer);
    addAndMakeVisible(tubeHeatMeter);

    // Big SQUEAL Stomp Trigger
    squealButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff881100));
    squealButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff3300));
    squealButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffffff));
    squealButton.addListener(this);
    addAndMakeVisible(squealButton);

    // Mode Selector
    modeBox.addItem("Mode: Amp Coupling", 1);
    modeBox.addItem("Mode: Larsen Howl", 2);
    modeBox.addItem("Mode: Comb Resonator", 3);
    modeBox.addItem("Mode: Chaos Screamer", 4);
    addAndMakeVisible(modeBox);

    // Harmonic Selector
    harmonicBox.addItem("Harmonic: Sub-Octave", 1);
    harmonicBox.addItem("Harmonic: Fundamental", 2);
    harmonicBox.addItem("Harmonic: Octave (+12st)", 3);
    harmonicBox.addItem("Harmonic: 5th (+19st)", 4);
    harmonicBox.addItem("Harmonic: 2nd Octave (+24st)", 5);
    harmonicBox.addItem("Harmonic: Major 3rd (+28st)", 6);
    harmonicBox.addItem("Harmonic: Manual / Free", 7);
    addAndMakeVisible(harmonicBox);

    // Saturation Selector
    saturationBox.addItem("Sat: Warm Tube", 1);
    saturationBox.addItem("Sat: JFET Scream", 2);
    saturationBox.addItem("Sat: Diode Clip", 3);
    saturationBox.addItem("Sat: Hard Clip", 4);
    saturationBox.addItem("Sat: Clean Limiter", 5);
    addAndMakeVisible(saturationBox);

    addAndMakeVisible(autoPitchToggle);
    addAndMakeVisible(polarityToggle);

    // Sliders
    setupRotarySlider(feedbackGainSlider, "FEEDBACK GAIN", "x");
    setupRotarySlider(distanceSlider, "DISTANCE", "ms");
    setupRotarySlider(phaseSlider, "PHASE", "°");
    setupRotarySlider(manualFreqSlider, "MANUAL FREQ", "Hz");
    setupRotarySlider(fineTuneSlider, "FINE TUNE", "c");

    setupRotarySlider(bloomRiseSlider, "BLOOM SWELL", "ms");
    setupRotarySlider(duckingSlider, "ATTACK DUCK", "%");
    setupRotarySlider(noiseExciterSlider, "NOISE EXCITER", "%");

    setupRotarySlider(lowCutSlider, "LOW CUT", "Hz");
    setupRotarySlider(highCutSlider, "HIGH CUT", "Hz");
    setupRotarySlider(peakResonanceHzSlider, "HOWL FREQ", "Hz");
    setupRotarySlider(peakResonanceQSlider, "HOWL Q", "");
    setupRotarySlider(peakResonanceGainSlider, "HOWL BOOST", "dB");

    setupRotarySlider(driveSlider, "TUBE DRIVE", "");
    setupRotarySlider(dryGainSlider, "DRY MIX", "");
    setupRotarySlider(wetGainSlider, "FEEDBACK WET", "");
    setupRotarySlider(outputCeilingSlider, "SAFETY LIMIT", "");

    // APVTS Attachments
    auto& apvts = audioProcessor.getAPVTS();
    modeAttachment = std::make_unique<ComboBoxAttachment>(apvts, "mode", modeBox);
    harmonicAttachment = std::make_unique<ComboBoxAttachment>(apvts, "harmonic", harmonicBox);
    saturationAttachment = std::make_unique<ComboBoxAttachment>(apvts, "saturation", saturationBox);

    onlyFeedbackAttachment = std::make_unique<ButtonAttachment>(apvts, "onlyFeedback", onlyFeedbackToggle);
    autoPitchAttachment = std::make_unique<ButtonAttachment>(apvts, "autoPitchTrack", autoPitchToggle);
    polarityAttachment = std::make_unique<ButtonAttachment>(apvts, "polarityInvert", polarityToggle);

    feedbackGainAttachment = std::make_unique<SliderAttachment>(apvts, "feedbackGain", feedbackGainSlider);
    distanceAttachment = std::make_unique<SliderAttachment>(apvts, "distanceMs", distanceSlider);
    phaseAttachment = std::make_unique<SliderAttachment>(apvts, "phaseDegrees", phaseSlider);
    manualFreqAttachment = std::make_unique<SliderAttachment>(apvts, "manualFreq", manualFreqSlider);
    fineTuneAttachment = std::make_unique<SliderAttachment>(apvts, "fineTune", fineTuneSlider);

    bloomRiseAttachment = std::make_unique<SliderAttachment>(apvts, "bloomRiseMs", bloomRiseSlider);
    duckingAttachment = std::make_unique<SliderAttachment>(apvts, "duckingAmount", duckingSlider);
    noiseExciterAttachment = std::make_unique<SliderAttachment>(apvts, "noiseExciter", noiseExciterSlider);

    lowCutAttachment = std::make_unique<SliderAttachment>(apvts, "lowCut", lowCutSlider);
    highCutAttachment = std::make_unique<SliderAttachment>(apvts, "highCut", highCutSlider);
    peakResonanceHzAttachment = std::make_unique<SliderAttachment>(apvts, "peakResonanceHz", peakResonanceHzSlider);
    peakResonanceQAttachment = std::make_unique<SliderAttachment>(apvts, "peakResonanceQ", peakResonanceQSlider);
    peakResonanceGainAttachment = std::make_unique<SliderAttachment>(apvts, "peakResonanceGainDb", peakResonanceGainSlider);

    driveAttachment = std::make_unique<SliderAttachment>(apvts, "drive", driveSlider);
    dryGainAttachment = std::make_unique<SliderAttachment>(apvts, "dryGain", dryGainSlider);
    wetGainAttachment = std::make_unique<SliderAttachment>(apvts, "wetGain", wetGainSlider);
    outputCeilingAttachment = std::make_unique<SliderAttachment>(apvts, "outputCeiling", outputCeilingSlider);

    setSize(860, 680);
}

OopsAllFeedbackAudioProcessorEditor::~OopsAllFeedbackAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void OopsAllFeedbackAudioProcessorEditor::setupRotarySlider(juce::Slider& slider, const juce::String& name, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setTextValueSuffix(" " + suffix);
    slider.setName(name);
    addAndMakeVisible(slider);

    auto label = std::make_unique<juce::Label>("", name);
    label->setFont(juce::Font(10.0f, juce::Font::bold));
    label->setJustificationType(juce::Justification::centred);
    label->setColour(juce::Label::textColourId, juce::Colour(0xff99a3b5));
    label->attachToComponent(&slider, false);
    addAndMakeVisible(label.get());
    sliderLabels.push_back(std::move(label));
}

void OopsAllFeedbackAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &squealButton)
    {
        audioProcessor.getFeedbackEngine().triggerSquealBurst(1.0f);
    }
}

void OopsAllFeedbackAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background gradient
    juce::ColourGradient bg(juce::Colour(0xff161820), 0.0f, 0.0f,
                            juce::Colour(0xff0c0d11), 0.0f, static_cast<float>(getHeight()), false);
    g.setGradientFill(bg);
    g.fillAll();

    // Draw Oops All Feedback Icon / Logo in Header
    if (logoImage.isValid())
    {
        g.drawImageWithin(logoImage, 16, 6, 42, 42, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }

    // Section bounding boxes
    auto drawSection = [&](juce::Rectangle<float> r, const juce::String& title, juce::Colour headerCol) {
        g.setColour(juce::Colour(0xff12141a));
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(juce::Colour(0xff262b36));
        g.drawRoundedRectangle(r, 6.0f, 1.0f);

        // Section Title Header
        g.setColour(headerCol);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(title, r.getX() + 8.0f, r.getY() + 4.0f, r.getWidth() - 16.0f, 16.0f, juce::Justification::left);
    };

    // Feedback Engine Box
    drawSection({ 16.0f, 210.0f, 260.0f, 220.0f }, "FEEDBACK LOOP & DISTANCE", juce::Colour(0xffff7700));

    // Resonator & Harmonic Tuning Box
    drawSection({ 286.0f, 210.0f, 280.0f, 220.0f }, "HARMONIC RESONATOR & PITCH", juce::Colour(0xff00d4ff));

    // Dynamic Bloom & Exciter Box
    drawSection({ 576.0f, 210.0f, 268.0f, 220.0f }, "DYNAMIC BLOOM & EXCITERS", juce::Colour(0xffff9933));

    // Tone & Sculpting Box
    drawSection({ 16.0f, 440.0f, 410.0f, 220.0f }, "TONE & RESONANCE FILTERS", juce::Colour(0xff44ffaa));

    // Saturation & Output Box
    drawSection({ 436.0f, 440.0f, 408.0f, 220.0f }, "SATURATION & OUTPUT SAFETY", juce::Colour(0xffff4455));
}

void OopsAllFeedbackAudioProcessorEditor::resized()
{
    // Header Bar
    titleLabel.setBounds(66, 8, 240, 22);
    subtitleLabel.setBounds(66, 28, 320, 16);

    presetBox.setBounds(390, 14, 180, 26);
    onlyFeedbackToggle.setBounds(580, 14, 150, 26);

    // Visualizers Deck
    spectrumVisualizer.setBounds(16, 56, 470, 140);
    distanceVisualizer.setBounds(496, 56, 140, 140);
    tubeHeatMeter.setBounds(646, 56, 50, 140);
    squealButton.setBounds(706, 56, 138, 140);

    // --- Row 1: Section Boxes (y = 210) ---

    // 1. Feedback Loop Box (x = 16, w = 260)
    modeBox.setBounds(26, 235, 140, 24);
    polarityToggle.setBounds(172, 235, 95, 24);
    feedbackGainSlider.setBounds(24, 275, 76, 85);
    distanceSlider.setBounds(104, 275, 76, 85);
    phaseSlider.setBounds(184, 275, 76, 85);

    // 2. Harmonic Resonator Box (x = 286, w = 280)
    harmonicBox.setBounds(296, 235, 150, 24);
    autoPitchToggle.setBounds(452, 235, 105, 24);
    manualFreqSlider.setBounds(320, 275, 80, 85);
    fineTuneSlider.setBounds(440, 275, 80, 85);

    // 3. Dynamic Bloom Box (x = 576, w = 268)
    bloomRiseSlider.setBounds(586, 265, 76, 85);
    duckingSlider.setBounds(672, 265, 76, 85);
    noiseExciterSlider.setBounds(758, 265, 76, 85);

    // --- Row 2: Section Boxes (y = 440) ---

    // 4. Tone Filters Box (x = 16, w = 410)
    lowCutSlider.setBounds(24, 480, 72, 85);
    highCutSlider.setBounds(102, 480, 72, 85);
    peakResonanceHzSlider.setBounds(180, 480, 72, 85);
    peakResonanceQSlider.setBounds(258, 480, 72, 85);
    peakResonanceGainSlider.setBounds(336, 480, 72, 85);

    // 5. Saturation & Output Box (x = 436, w = 408)
    saturationBox.setBounds(446, 465, 140, 24);
    driveSlider.setBounds(446, 505, 72, 85);
    dryGainSlider.setBounds(524, 505, 72, 85);
    wetGainSlider.setBounds(602, 505, 72, 85);
    outputCeilingSlider.setBounds(680, 505, 72, 85);
}

} // namespace OAF
