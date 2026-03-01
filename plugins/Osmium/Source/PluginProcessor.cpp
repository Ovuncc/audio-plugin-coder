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
    // Saturator function is initialized in prepareToPlay.
}

OsmiumAudioProcessor::~OsmiumAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OsmiumAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Intensity (The Core)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::intensity, 1),
        "Osmium Core",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // Output Gain
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::outputGain, 1),
        "Output",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f));

    // Bypass
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::bypass, 1),
        "Bypass",
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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int OsmiumAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OsmiumAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String OsmiumAudioProcessor::getProgramName (int index)
{
    return {};
}

void OsmiumAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void OsmiumAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    const auto numChannels = static_cast<int>(spec.numChannels);

    // Prepare multiband filters. Slightly higher split keeps more body in the low band.
    lowpassFilter.prepare(spec);
    lowpassFilter.setCutoffFrequency(220.0f);
    lowpassFilter.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    
    highpassFilter.prepare(spec);
    highpassFilter.setCutoffFrequency(220.0f);
    highpassFilter.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    
    // Prepare low band processors
    lowBandTransientShaper.prepare(sampleRate, samplesPerBlock, numChannels);
    lowBandLimiter.prepare(spec);
    lowBandLimiter.setThreshold(-0.15f);
    lowBandLimiter.setRelease(180.0f);
    
    // Prepare high band processors
    highBandTransientShaper.prepare(sampleRate, samplesPerBlock, numChannels);
    highBandDrive.prepare(spec);
    highBandSaturator.prepare(spec);
    highBandSaturator.functionToUse = [](float x) { return x; };
    
    // Output
    outputGain.prepare(spec);
    outputLimiter.prepare(spec);
    outputLimiter.setThreshold(-0.1f);
    outputLimiter.setRelease(80.0f);
    
    // Allocate multiband buffers
    lowBandBuffer.setSize(spec.numChannels, samplesPerBlock);
    highBandBuffer.setSize(spec.numChannels, samplesPerBlock);
    
    // Smoothing
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

    // This checks if the input layout matches the output layout
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
        
    // Bypass check
    if (*apvts.getRawParameterValue(ParameterIDs::bypass) > 0.5f)
        return;

    // Parameter Updates
    float intensityTarget = *apvts.getRawParameterValue(ParameterIDs::intensity);
    float outputTarget = *apvts.getRawParameterValue(ParameterIDs::outputGain);
    
    smoothedIntensity.setTargetValue(intensityTarget);
    smoothedOutput.setTargetValue(outputTarget);
    smoothedIntensity.skip(buffer.getNumSamples());
    smoothedOutput.skip(buffer.getNumSamples());
    
    float currentIntensity = smoothedIntensity.getCurrentValue();
    float currentOutput = smoothedOutput.getCurrentValue();
    const float intensity = juce::jlimit(0.0f, 1.0f, currentIntensity);
    const float densityRegion = juce::jlimit(0.0f, 1.0f, (intensity - 0.60f) / 0.40f);
    const float density = densityRegion * densityRegion;

    // MULTIBAND PROCESSING
    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();
    
    // Ensure buffers are sized correctly
    lowBandBuffer.setSize(numChannels, numSamples, false, false, true);
    highBandBuffer.setSize(numChannels, numSamples, false, false, true);
    
    // Copy input to both band buffers
    for (int ch = 0; ch < numChannels; ++ch) {
        lowBandBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
        highBandBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }
    
    // Split into bands using Linkwitz-Riley filters
    juce::dsp::AudioBlock<float> lowBlock(lowBandBuffer);
    juce::dsp::AudioBlock<float> highBlock(highBandBuffer);
    juce::dsp::ProcessContextReplacing<float> lowContext(lowBlock);
    juce::dsp::ProcessContextReplacing<float> highContext(highBlock);
    
    const float crossoverHz = juce::jmap(intensity, 220.0f, 600.0f);
    lowpassFilter.setCutoffFrequency(crossoverHz);
    highpassFilter.setCutoffFrequency(crossoverHz);

    lowpassFilter.process(lowContext);   // Low band: 20-crossover Hz
    highpassFilter.process(highContext); // High band: crossover Hz+
    
    // Low band: emphasize slam and body without thinning the tail.
    float lowAttackBoost = juce::jmap(intensity, 0.0f, 7.3563636f) + density * 4.5436364f;
    float lowPostAttackComp = juce::jmap(intensity, 0.0f, 3.01875f) + density * 3.88125f;
    float lowAttackTimeMs = juce::jmap(density, 14.0f, 30.8f);
    float lowCompTimeMs = juce::jmap(density, 24.0f, 68.5f);
    lowBandTransientShaper.setParameters(lowAttackBoost, lowPostAttackComp, lowAttackTimeMs, lowCompTimeMs);
    lowBandTransientShaper.process(lowBandBuffer);

    const float lowBodyLiftDb = juce::jmap(intensity, 0.0f, 3.9666667f) + density * 7.9333333f;
    const float lowBodyLift = juce::Decibels::decibelsToGain(lowBodyLiftDb);
    if (lowBodyLift > 1.0f) {
        for (int ch = 0; ch < numChannels; ++ch)
            lowBandBuffer.applyGain(ch, 0, numSamples, lowBodyLift);
    }

    const float lowSatMix = density * 0.39f;
    if (lowSatMix > 0.0f) {
        const float lowSatDrive = 1.0f + density * 0.94f;
        for (int ch = 0; ch < numChannels; ++ch) {
            auto* lowBandData = lowBandBuffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                const float dry = lowBandData[i];
                const float shaped = std::tanh(dry * lowSatDrive);
                lowBandData[i] = dry + (shaped - dry) * lowSatMix;
            }
        }
    }
    
    // Keep low end peaks in check.
    lowBandLimiter.process(lowContext);
    
    // High band: still aggressive near the top, but less dominant.
    float highAttackBoost = juce::jmap(intensity, 0.0f, 8.5615385f) + density * 7.3384615f;
    float highPostAttackComp = juce::jmap(intensity, 0.0f, 2.3368421f) + density * 5.0631579f;
    float highAttackTimeMs = juce::jmap(density, 6.0f, 12.7f);
    float highCompTimeMs = juce::jmap(density, 14.0f, 38.1f);
    highBandTransientShaper.setParameters(highAttackBoost, highPostAttackComp, highAttackTimeMs, highCompTimeMs);
    highBandTransientShaper.process(highBandBuffer);
    
    float driveDb = juce::jmap(intensity, 0.0f, 3.99f) + density * 5.51f;
    highBandDrive.setGainDecibels(driveDb);
    highBandDrive.process(highContext);
    
    const float saturationMix = juce::jlimit(0.0f, 0.46f, juce::jmap(intensity, 0.0f, 0.1686667f) + density * 0.2913333f);
    const float saturationDrive = juce::jmap(intensity, 1.0f, 1.7392f) + density * 0.9408f;
    if (saturationMix > 0.0f) {
        for (int ch = 0; ch < numChannels; ++ch) {
            auto* highBandData = highBandBuffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                const float dry = highBandData[i];
                const float shaped = std::tanh(dry * saturationDrive);
                highBandData[i] = dry + (shaped - dry) * saturationMix;
            }
        }
    }

    const float highBandTrimDb = -(juce::jmap(intensity, 0.1f, 0.8090909f) + density * 3.1909091f);
    const float highBandTrim = juce::Decibels::decibelsToGain(highBandTrimDb);
    if (highBandTrim < 1.0f) {
        for (int ch = 0; ch < numChannels; ++ch)
            highBandBuffer.applyGain(ch, 0, numSamples, highBandTrim);
    }
    
    // Recombine bands.
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* output = buffer.getWritePointer(ch);
        auto* low = lowBandBuffer.getReadPointer(ch);
        auto* high = highBandBuffer.getReadPointer(ch);
        
        for (int i = 0; i < numSamples; ++i)
            output[i] = low[i] + high[i];
    }
    
    // Output gain and final peak control.
    juce::dsp::AudioBlock<float> outputBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> outputContext(outputBlock);
    
    float autoMakeup = -driveDb * (0.10f + density * 0.08f);
    float bodyHoldDb = juce::jmap(intensity, 0.0f, 0.2f) + density * 0.8f;
    float finalOutputDb = autoMakeup + bodyHoldDb + currentOutput;
    outputGain.setGainDecibels(finalOutputDb);
    outputGain.process(outputContext);
    outputLimiter.process(outputContext);
}

//==============================================================================
bool OsmiumAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
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
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OsmiumAudioProcessor();
}
