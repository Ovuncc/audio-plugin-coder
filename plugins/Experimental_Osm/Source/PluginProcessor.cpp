#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OsmiumAudioProcessor::OsmiumAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

OsmiumAudioProcessor::~OsmiumAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OsmiumAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Core controls
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::intensity, 1),
        "Osmium Core",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::outputGain, 1),
        "Output",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::bypass, 1),
        "Bypass",
        false));

    // Experimental mode + crossover
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::expManualMode, 1),
        "EXP Manual Mode",
        false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expCrossoverHz, 1),
        "EXP Crossover (Hz)",
        juce::NormalisableRange<float>(80.0f, 600.0f, 1.0f, 0.4f),
        220.0f));

    // Low band
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowAttackBoostDb, 1),
        "EXP Low Attack Boost (dB)",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f),
        5.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowAttackTimeMs, 1),
        "EXP Low Attack Time (ms)",
        juce::NormalisableRange<float>(2.0f, 40.0f, 0.1f, 0.45f),
        14.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowPostCompDb, 1),
        "EXP Low Post Attack Comp (dB)",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f),
        3.2f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowCompTimeMs, 1),
        "EXP Low Compression Time (ms)",
        juce::NormalisableRange<float>(5.0f, 120.0f, 0.1f, 0.4f),
        24.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowBodyLiftDb, 1),
        "EXP Low Body Lift (dB)",
        juce::NormalisableRange<float>(-3.0f, 12.0f, 0.1f),
        3.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowSatMix, 1),
        "EXP Low Saturation Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.45f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowSatDrive, 1),
        "EXP Low Saturation Drive",
        juce::NormalisableRange<float>(1.0f, 3.0f, 0.01f),
        1.8f));

    // High band
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighAttackBoostDb, 1),
        "EXP High Attack Boost (dB)",
        juce::NormalisableRange<float>(0.0f, 16.0f, 0.1f),
        7.8f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighAttackTimeMs, 1),
        "EXP High Attack Time (ms)",
        juce::NormalisableRange<float>(2.0f, 40.0f, 0.1f, 0.45f),
        6.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighPostCompDb, 1),
        "EXP High Post Attack Comp (dB)",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f),
        3.8f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighCompTimeMs, 1),
        "EXP High Compression Time (ms)",
        juce::NormalisableRange<float>(5.0f, 120.0f, 0.1f, 0.4f),
        14.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighDriveDb, 1),
        "EXP High Drive (dB)",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f),
        10.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighSatMix, 1),
        "EXP High Saturation Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.60f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighSatDrive, 1),
        "EXP High Saturation Drive",
        juce::NormalisableRange<float>(1.0f, 3.0f, 0.01f),
        2.25f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighTrimDb, 1),
        "EXP High Band Trim (dB)",
        juce::NormalisableRange<float>(-24.0f, 0.0f, 0.1f),
        -2.2f));

    // Output limiter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLimiterThresholdDb, 1),
        "EXP Limiter Threshold (dB)",
        juce::NormalisableRange<float>(-6.0f, 0.0f, 0.1f),
        -0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLimiterReleaseMs, 1),
        "EXP Limiter Release (ms)",
        juce::NormalisableRange<float>(10.0f, 300.0f, 0.1f, 0.45f),
        80.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::expMuteLowBand, 1),
        "EXP Mute Low Band",
        false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::expMuteHighBand, 1),
        "EXP Mute High Band",
        false));

    return layout;
}

const juce::String OsmiumAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OsmiumAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool OsmiumAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool OsmiumAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double OsmiumAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OsmiumAudioProcessor::getNumPrograms()
{
    return 1;
}

int OsmiumAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OsmiumAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String OsmiumAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void OsmiumAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void OsmiumAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    const auto numChannels = static_cast<int>(spec.numChannels);

    lowpassFilter.prepare(spec);
    lowpassFilter.setCutoffFrequency(220.0f);
    lowpassFilter.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);

    highpassFilter.prepare(spec);
    highpassFilter.setCutoffFrequency(220.0f);
    highpassFilter.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    lowBandTransientShaper.prepare(sampleRate, samplesPerBlock, numChannels);
    lowBandLimiter.prepare(spec);
    lowBandLimiter.setThreshold(-0.15f);
    lowBandLimiter.setRelease(180.0f);

    highBandTransientShaper.prepare(sampleRate, samplesPerBlock, numChannels);
    highBandDrive.prepare(spec);
    highBandSaturator.prepare(spec);
    highBandSaturator.functionToUse = [](float x) { return x; };

    outputGain.prepare(spec);
    outputLimiter.prepare(spec);
    outputLimiter.setThreshold(-0.5f);
    outputLimiter.setRelease(80.0f);

    lowBandBuffer.setSize(spec.numChannels, samplesPerBlock);
    highBandBuffer.setSize(spec.numChannels, samplesPerBlock);

    smoothedIntensity.reset(sampleRate, 0.05);
    smoothedOutput.reset(sampleRate, 0.05);
}

void OsmiumAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OsmiumAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void OsmiumAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (*apvts.getRawParameterValue(ParameterIDs::bypass) > 0.5f)
        return;

    const auto getParam = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };

    const float intensityTarget = getParam(ParameterIDs::intensity);
    const float outputTarget = getParam(ParameterIDs::outputGain);
    const bool manualMode = true; // Core macro is intentionally locked out in Experimental_Osm.
    const bool muteLowBand = getParam(ParameterIDs::expMuteLowBand) > 0.5f;
    const bool muteHighBand = getParam(ParameterIDs::expMuteHighBand) > 0.5f;

    smoothedIntensity.setTargetValue(intensityTarget);
    smoothedOutput.setTargetValue(outputTarget);
    smoothedIntensity.skip(buffer.getNumSamples());
    smoothedOutput.skip(buffer.getNumSamples());

    const float currentIntensity = smoothedIntensity.getCurrentValue();
    const float currentOutput = smoothedOutput.getCurrentValue();
    const float intensity = juce::jlimit(0.0f, 1.0f, currentIntensity);
    const float densityRegion = juce::jlimit(0.0f, 1.0f, (intensity - 0.60f) / 0.40f);
    const float density = densityRegion * densityRegion;

    // Macro defaults (when EXP manual mode is off)
    float crossoverHz = 220.0f;
    float lowAttackBoost = juce::jmap(intensity, 0.0f, 3.4f) + density * 2.1f;
    float lowPostAttackComp = juce::jmap(intensity, 0.0f, 1.4f) + density * 1.8f;
    float lowAttackTimeMs = juce::jmap(density, 22.0f, 14.0f);
    float lowCompTimeMs = juce::jmap(density, 34.0f, 20.0f);
    float lowBodyLiftDb = juce::jmap(intensity, 0.0f, 1.1f) + density * 2.2f;
    float lowSatMix = density * 0.45f;
    float lowSatDrive = 1.0f + density * 0.8f;

    float highAttackBoost = juce::jmap(intensity, 0.0f, 4.2f) + density * 3.6f;
    float highPostAttackComp = juce::jmap(intensity, 0.0f, 1.2f) + density * 2.6f;
    float highAttackTimeMs = juce::jmap(density, 12.0f, 6.0f);
    float highCompTimeMs = juce::jmap(density, 24.0f, 14.0f);
    float highDriveDb = juce::jmap(intensity, 0.0f, 4.2f) + density * 5.8f;
    float highSatMix = juce::jlimit(0.0f, 0.68f, juce::jmap(intensity, 0.0f, 0.22f) + density * 0.38f);
    float highSatDrive = juce::jmap(intensity, 1.0f, 1.55f) + density * 0.7f;
    float highBandTrimDb = -(juce::jmap(intensity, 0.0f, 0.4f) + density * 1.8f);

    float limiterThresholdDb = -0.5f;
    float limiterReleaseMs = 80.0f;
    float autoMakeupDb = -highDriveDb * (0.10f + density * 0.08f);
    float bodyHoldDb = juce::jmap(intensity, 0.0f, 0.2f) + density * 0.8f;

    // Manual mode values (core macro bypass)
    if (manualMode) {
        crossoverHz = getParam(ParameterIDs::expCrossoverHz);

        lowAttackBoost = getParam(ParameterIDs::expLowAttackBoostDb);
        lowPostAttackComp = getParam(ParameterIDs::expLowPostCompDb);
        lowAttackTimeMs = getParam(ParameterIDs::expLowAttackTimeMs);
        lowCompTimeMs = getParam(ParameterIDs::expLowCompTimeMs);
        lowBodyLiftDb = getParam(ParameterIDs::expLowBodyLiftDb);
        lowSatMix = getParam(ParameterIDs::expLowSatMix);
        lowSatDrive = getParam(ParameterIDs::expLowSatDrive);

        highAttackBoost = getParam(ParameterIDs::expHighAttackBoostDb);
        highPostAttackComp = getParam(ParameterIDs::expHighPostCompDb);
        highAttackTimeMs = getParam(ParameterIDs::expHighAttackTimeMs);
        highCompTimeMs = getParam(ParameterIDs::expHighCompTimeMs);
        highDriveDb = getParam(ParameterIDs::expHighDriveDb);
        highSatMix = getParam(ParameterIDs::expHighSatMix);
        highSatDrive = getParam(ParameterIDs::expHighSatDrive);
        highBandTrimDb = getParam(ParameterIDs::expHighTrimDb);

        limiterThresholdDb = getParam(ParameterIDs::expLimiterThresholdDb);
        limiterReleaseMs = getParam(ParameterIDs::expLimiterReleaseMs);
        autoMakeupDb = -highDriveDb * 0.04f;
        bodyHoldDb = 0.0f;
    }

    crossoverHz = juce::jlimit(80.0f, 600.0f, crossoverHz);
    lowSatMix = juce::jlimit(0.0f, 1.0f, lowSatMix);
    lowSatDrive = juce::jlimit(1.0f, 3.0f, lowSatDrive);
    highSatMix = juce::jlimit(0.0f, 1.0f, highSatMix);
    highSatDrive = juce::jlimit(1.0f, 3.0f, highSatDrive);
    highBandTrimDb = juce::jlimit(-24.0f, 0.0f, highBandTrimDb);
    limiterThresholdDb = juce::jlimit(-6.0f, 0.0f, limiterThresholdDb);
    limiterReleaseMs = juce::jlimit(10.0f, 300.0f, limiterReleaseMs);

    lowpassFilter.setCutoffFrequency(crossoverHz);
    highpassFilter.setCutoffFrequency(crossoverHz);
    outputLimiter.setThreshold(limiterThresholdDb);
    outputLimiter.setRelease(limiterReleaseMs);

    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    lowBandBuffer.setSize(numChannels, numSamples, false, false, true);
    highBandBuffer.setSize(numChannels, numSamples, false, false, true);

    for (int ch = 0; ch < numChannels; ++ch) {
        lowBandBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
        highBandBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    juce::dsp::AudioBlock<float> lowBlock(lowBandBuffer);
    juce::dsp::AudioBlock<float> highBlock(highBandBuffer);
    juce::dsp::ProcessContextReplacing<float> lowContext(lowBlock);
    juce::dsp::ProcessContextReplacing<float> highContext(highBlock);

    lowpassFilter.process(lowContext);   // Low band: 20-crossover Hz
    highpassFilter.process(highContext); // High band: crossover Hz+

    lowBandTransientShaper.setParameters(lowAttackBoost, lowPostAttackComp, lowAttackTimeMs, lowCompTimeMs);
    lowBandTransientShaper.process(lowBandBuffer);

    const float lowBodyLift = juce::Decibels::decibelsToGain(lowBodyLiftDb);
    if (std::abs(lowBodyLift - 1.0f) > 1.0e-5f) {
        for (int ch = 0; ch < numChannels; ++ch)
            lowBandBuffer.applyGain(ch, 0, numSamples, lowBodyLift);
    }

    if (lowSatMix > 0.0f) {
        for (int ch = 0; ch < numChannels; ++ch) {
            auto* lowBandData = lowBandBuffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                const float dry = lowBandData[i];
                const float shaped = std::tanh(dry * lowSatDrive);
                lowBandData[i] = dry + (shaped - dry) * lowSatMix;
            }
        }
    }

    lowBandLimiter.process(lowContext);

    highBandTransientShaper.setParameters(highAttackBoost, highPostAttackComp, highAttackTimeMs, highCompTimeMs);
    highBandTransientShaper.process(highBandBuffer);

    highBandDrive.setGainDecibels(highDriveDb);
    highBandDrive.process(highContext);

    if (highSatMix > 0.0f) {
        for (int ch = 0; ch < numChannels; ++ch) {
            auto* highBandData = highBandBuffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                const float dry = highBandData[i];
                const float shaped = std::tanh(dry * highSatDrive);
                highBandData[i] = dry + (shaped - dry) * highSatMix;
            }
        }
    }

    const float highBandTrim = juce::Decibels::decibelsToGain(highBandTrimDb);
    if (std::abs(highBandTrim - 1.0f) > 1.0e-5f) {
        for (int ch = 0; ch < numChannels; ++ch)
            highBandBuffer.applyGain(ch, 0, numSamples, highBandTrim);
    }

    if (muteLowBand)
        lowBandBuffer.clear();
    if (muteHighBand)
        highBandBuffer.clear();

    for (int ch = 0; ch < numChannels; ++ch) {
        auto* output = buffer.getWritePointer(ch);
        auto* low = lowBandBuffer.getReadPointer(ch);
        auto* high = highBandBuffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            output[i] = low[i] + high[i];
    }

    juce::dsp::AudioBlock<float> outputBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> outputContext(outputBlock);

    const float finalOutputDb = autoMakeupDb + bodyHoldDb + currentOutput;
    outputGain.setGainDecibels(finalOutputDb);
    outputGain.process(outputContext);
    outputLimiter.process(outputContext);
}

//==============================================================================
bool OsmiumAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OsmiumAudioProcessor::createEditor()
{
    return new OsmiumAudioProcessorEditor (*this);
}

//==============================================================================
void OsmiumAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void OsmiumAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OsmiumAudioProcessor();
}
