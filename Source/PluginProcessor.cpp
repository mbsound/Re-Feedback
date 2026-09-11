#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace OAF
{

OopsAllFeedbackAudioProcessor::OopsAllFeedbackAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

OopsAllFeedbackAudioProcessor::~OopsAllFeedbackAudioProcessor()
{
}

const juce::String OopsAllFeedbackAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OopsAllFeedbackAudioProcessor::acceptsMidi() const { return true; }
bool OopsAllFeedbackAudioProcessor::producesMidi() const { return false; }
bool OopsAllFeedbackAudioProcessor::isMidiEffect() const { return false; }
double OopsAllFeedbackAudioProcessor::getTailLengthSeconds() const { return 2.0; }

int OopsAllFeedbackAudioProcessor::getNumPrograms() { return getPresetNames().size(); }
int OopsAllFeedbackAudioProcessor::getCurrentProgram() { return currentProgram; }
void OopsAllFeedbackAudioProcessor::setCurrentProgram(int index) { loadPreset(index); }
const juce::String OopsAllFeedbackAudioProcessor::getProgramName(int index)
{
    auto names = getPresetNames();
    if (index >= 0 && index < names.size()) return names[index];
    return {};
}
void OopsAllFeedbackAudioProcessor::changeProgramName(int, const juce::String&) {}

void OopsAllFeedbackAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    feedbackEngine.prepare(sampleRate, samplesPerBlock);
    updateParameters();
}

void OopsAllFeedbackAudioProcessor::releaseResources()
{
    feedbackEngine.reset();
}

bool OopsAllFeedbackAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void OopsAllFeedbackAudioProcessor::updateParameters()
{
    FeedbackParameters params;

    int modeIdx = static_cast<int>(*apvts.getRawParameterValue("mode"));
    params.mode = static_cast<FeedbackMode>(modeIdx);

    int harmIdx = static_cast<int>(*apvts.getRawParameterValue("harmonic"));
    params.harmonic = static_cast<HarmonicInterval>(harmIdx);

    int satIdx = static_cast<int>(*apvts.getRawParameterValue("saturation"));
    params.saturation = static_cast<SaturationType>(satIdx);

    params.feedbackGain = *apvts.getRawParameterValue("feedbackGain");
    params.distanceMs = *apvts.getRawParameterValue("distanceMs");
    params.phaseDegrees = *apvts.getRawParameterValue("phaseDegrees");
    params.polarityInvert = *apvts.getRawParameterValue("polarityInvert") > 0.5f;

    params.autoPitchTrack = *apvts.getRawParameterValue("autoPitchTrack") > 0.5f;
    params.manualFreqHz = *apvts.getRawParameterValue("manualFreq");
    params.fineTuneCents = *apvts.getRawParameterValue("fineTune");

    params.filterLowCutHz = *apvts.getRawParameterValue("lowCut");
    params.filterHighCutHz = *apvts.getRawParameterValue("highCut");
    params.peakResonanceHz = *apvts.getRawParameterValue("peakResonanceHz");
    params.peakResonanceQ = *apvts.getRawParameterValue("peakResonanceQ");
    params.peakResonanceGainDb = *apvts.getRawParameterValue("peakResonanceGainDb");

    params.bloomRiseMs = *apvts.getRawParameterValue("bloomRiseMs");
    params.duckingAmount = *apvts.getRawParameterValue("duckingAmount");
    params.noiseExciter = *apvts.getRawParameterValue("noiseExciter");
    params.squealTrigger = *apvts.getRawParameterValue("squealTrigger") > 0.5f;

    params.onlyFeedback = *apvts.getRawParameterValue("onlyFeedback") > 0.5f;
    params.dryGain = *apvts.getRawParameterValue("dryGain");
    params.wetGain = *apvts.getRawParameterValue("wetGain");
    params.drive = *apvts.getRawParameterValue("drive");
    params.outputCeiling = *apvts.getRawParameterValue("outputCeiling");

    feedbackEngine.setParameters(params);
}

void OopsAllFeedbackAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Handle MIDI triggers (e.g. MIDI note on can trigger squeal burst or set manual frequency)
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            float noteFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(msg.getNoteNumber()));
            auto* pManual = apvts.getParameter("manualFreq");
            if (pManual != nullptr)
                pManual->setValueNotifyingHost(pManual->convertTo0to1(noteFreq));

            feedbackEngine.triggerSquealBurst(msg.getFloatVelocity());
        }
    }

    updateParameters();

    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = totalNumInputChannels > 1 ? buffer.getWritePointer(1) : channelDataL;

    feedbackEngine.processStereo(channelDataL, channelDataR, buffer.getNumSamples());

    if (onAudioBlockProcessed)
    {
        onAudioBlockProcessed(channelDataL, buffer.getNumSamples());
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout OopsAllFeedbackAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Feedback Gain
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"feedbackGain", 1}, "Feedback Gain",
        juce::NormalisableRange<float>(0.0f, 3.0f, 0.01f, 0.7f), 1.25f));

    // Mode
    juce::StringArray modeChoices = { "Amp Coupling", "Larsen Howl", "Comb Resonator", "Chaos Screamer" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"mode", 1}, "Feedback Mode", modeChoices, 0));

    // Harmonic Interval
    juce::StringArray harmChoices = { "Sub-Octave (-12st)", "Fundamental (1x)", "Octave (+12st)", "Fifth (+19st)", "2nd Octave (+24st)", "Major 3rd (+28st)", "Manual / Free" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"harmonic", 1}, "Harmonic Interval", harmChoices, 2));

    // Saturation
    juce::StringArray satChoices = { "Tube", "JFET Scream", "Diode Clip", "Hard Clip", "Clean" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"saturation", 1}, "Saturation Type", satChoices, 0));

    // Physical Acoustic Distance & Phase
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"distanceMs", 1}, "Acoustic Distance",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.5f), 5.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"phaseDegrees", 1}, "Phase Angle",
        juce::NormalisableRange<float>(0.0f, 360.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"polarityInvert", 1}, "Phase Invert", false));

    // Pitch Tracking & Frequency
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"autoPitchTrack", 1}, "Auto Pitch Lock", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"manualFreq", 1}, "Manual Frequency",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 440.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"fineTune", 1}, "Fine Detune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.5f), 0.0f));

    // Tone Sculpting Filters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lowCut", 1}, "Low Cut",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.4f), 80.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"highCut", 1}, "High Cut",
        juce::NormalisableRange<float>(500.0f, 20000.0f, 1.0f, 0.4f), 8000.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"peakResonanceHz", 1}, "Resonant Howl Freq",
        juce::NormalisableRange<float>(100.0f, 12000.0f, 1.0f, 0.4f), 1800.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"peakResonanceQ", 1}, "Resonance Peak Q",
        juce::NormalisableRange<float>(0.5f, 35.0f, 0.1f, 0.5f), 4.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"peakResonanceGainDb", 1}, "Resonance Boost",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.5f), 6.0f));

    // Dynamics, Bloom, and Exciters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"bloomRiseMs", 1}, "Bloom Rise Time",
        juce::NormalisableRange<float>(5.0f, 2000.0f, 1.0f, 0.4f), 350.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"duckingAmount", 1}, "Attack Ducking",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"noiseExciter", 1}, "Seed Noise Exciter",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"squealTrigger", 1}, "Squeal Trigger", false));

    // Routing & Mix
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"onlyFeedback", 1}, "ONLY Feedback (Mute Dry)", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"dryGain", 1}, "Dry Level",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"wetGain", 1}, "Feedback Wet Level",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drive", 1}, "Tube Drive",
        juce::NormalisableRange<float>(1.0f, 10.0f, 0.1f), 2.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"outputCeiling", 1}, "Safety Limiter Ceiling",
        juce::NormalisableRange<float>(0.1f, 1.0f, 0.01f), 0.95f));

    return { params.begin(), params.end() };
}

juce::StringArray OopsAllFeedbackAudioProcessor::getPresetNames()
{
    return {
        "Hendrix Strat Bloom",
        "Cobain Feedback Wall",
        "PA Vocal Mic Howl (Larsen)",
        "Sub-Bass Acoustic Drone",
        "Harmonic 5th Shimmer",
        "2nd Octave Screamer",
        "Chaos Ring Mod Squeal",
        "Pure Feedback Solo (Only Feedback)"
    };
}

void OopsAllFeedbackAudioProcessor::loadPreset(int presetIndex)
{
    currentProgram = presetIndex;
    auto setP = [this](const juce::String& id, float val) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(val));
    };

    switch (presetIndex)
    {
        case 0: // Hendrix Strat Bloom
            setP("mode", 0.0f); // Amp Coupling
            setP("harmonic", 2.0f); // Octave
            setP("feedbackGain", 1.3f);
            setP("bloomRiseMs", 400.0f);
            setP("saturation", 0.0f); // Tube
            setP("drive", 2.8f);
            setP("onlyFeedback", 0.0f);
            setP("noiseExciter", 0.0f);
            break;

        case 1: // Cobain Feedback Wall
            setP("mode", 0.0f); // Amp Coupling
            setP("harmonic", 1.0f); // Fundamental
            setP("feedbackGain", 2.2f);
            setP("bloomRiseMs", 100.0f);
            setP("saturation", 1.0f); // JFET
            setP("drive", 6.0f);
            setP("onlyFeedback", 0.0f);
            setP("noiseExciter", 0.1f);
            break;

        case 2: // PA Vocal Mic Howl (Larsen)
            setP("mode", 1.0f); // Larsen Howl
            setP("distanceMs", 25.0f);
            setP("feedbackGain", 1.6f);
            setP("peakResonanceHz", 2800.0f);
            setP("peakResonanceQ", 12.0f);
            setP("peakResonanceGainDb", 12.0f);
            setP("saturation", 2.0f); // Diode
            setP("onlyFeedback", 0.0f);
            break;

        case 3: // Sub-Bass Acoustic Drone
            setP("mode", 2.0f); // Comb Resonator
            setP("harmonic", 0.0f); // Sub-Octave
            setP("feedbackGain", 1.8f);
            setP("lowCut", 30.0f);
            setP("highCut", 400.0f);
            setP("saturation", 0.0f);
            setP("onlyFeedback", 0.0f);
            break;

        case 4: // Harmonic 5th Shimmer
            setP("mode", 0.0f);
            setP("harmonic", 3.0f); // 5th (+19st)
            setP("feedbackGain", 1.5f);
            setP("bloomRiseMs", 600.0f);
            setP("saturation", 0.0f);
            setP("onlyFeedback", 0.0f);
            break;

        case 5: // 2nd Octave Screamer
            setP("mode", 2.0f);
            setP("harmonic", 4.0f); // 2nd Octave (+24st)
            setP("feedbackGain", 2.0f);
            setP("drive", 5.0f);
            setP("onlyFeedback", 0.0f);
            break;

        case 6: // Chaos Ring Mod Squeal
            setP("mode", 3.0f); // Chaos Screamer
            setP("feedbackGain", 2.5f);
            setP("saturation", 3.0f); // Hard clip
            setP("drive", 7.0f);
            setP("noiseExciter", 0.3f);
            setP("onlyFeedback", 0.0f);
            break;

        case 7: // Pure Feedback Solo (Only Feedback)
            setP("mode", 0.0f);
            setP("harmonic", 2.0f);
            setP("feedbackGain", 1.8f);
            setP("onlyFeedback", 1.0f); // 100% ONLY FEEDBACK
            setP("dryGain", 0.0f);
            setP("wetGain", 1.2f);
            break;
    }
}

void OopsAllFeedbackAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OopsAllFeedbackAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
            updateParameters();
        }
    }
}

juce::AudioProcessorEditor* OopsAllFeedbackAudioProcessor::createEditor()
{
    return new OopsAllFeedbackAudioProcessorEditor(*this);
}

bool OopsAllFeedbackAudioProcessor::hasEditor() const
{
    return true;
}

} // namespace OAF

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OAF::OopsAllFeedbackAudioProcessor();
}
