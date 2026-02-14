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
    // Saturator function will be initialized in prepareToPlay
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

    // Prepare multiband filters (Linkwitz-Riley 150Hz crossover)
    lowpassFilter.prepare(spec);
    lowpassFilter.setCutoffFrequency(150.0f);
    lowpassFilter.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    
    highpassFilter.prepare(spec);
    highpassFilter.setCutoffFrequency(150.0f);
    highpassFilter.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    
    // Prepare low band processors
    lowBandTransientShaper.prepare(sampleRate, samplesPerBlock);
    lowBandLimiter.prepare(spec);
    lowBandLimiter.setThreshold(-0.1f);
    lowBandLimiter.setRelease(100.0f);
    
    // Prepare high band processors
    highBandTransientShaper.prepare(sampleRate, samplesPerBlock);
    highBandDrive.prepare(spec);
    highBandSaturator.prepare(spec);
    highBandSaturator.functionToUse = [](float x) {
        return std::tanh(x * 1.5f); // Soft clipping
    };
    
    // Output
    outputGain.prepare(spec);
    
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
    
    lowpassFilter.process(lowContext);   // Low band: 20-150Hz
    highpassFilter.process(highContext); // High band: 150Hz+
    
    // === LOW BAND PROCESSING (20-150Hz) ===
    // Transient shaping only - NO drive, NO saturation
    float lowAttackBoost = juce::jmap(currentIntensity, 0.0f, 6.0f);  // 0dB → +6dB
    float lowSustainCut = juce::jmap(currentIntensity, 0.0f, -9.0f);  // 0dB → -9dB
    lowBandTransientShaper.setParameters(lowAttackBoost, lowSustainCut, 15.0f);
    lowBandTransientShaper.process(lowBandBuffer);
    
    // Gentle limiting to prevent sub clipping
    lowBandLimiter.process(lowContext);
    
    // === HIGH BAND PROCESSING (150Hz+) ===
    // Aggressive transient shaping + drive + saturation
    float highAttackBoost = juce::jmap(currentIntensity, 0.0f, 9.0f);   // 0dB → +9dB
    float highSustainCut = juce::jmap(currentIntensity, 0.0f, -12.0f);  // 0dB → -12dB
    highBandTransientShaper.setParameters(highAttackBoost, highSustainCut, 5.0f);
    highBandTransientShaper.process(highBandBuffer);
    
    // Drive (only on highs to prevent mud)
    float driveDb = juce::jmap(currentIntensity, 0.0f, 12.0f);
    highBandDrive.setGainDecibels(driveDb);
    highBandDrive.process(highContext);
    
    // Saturation (only on highs)
    highBandSaturator.process(highContext);
    
    // === RECOMBINE BANDS ===
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* output = buffer.getWritePointer(ch);
        auto* low = lowBandBuffer.getReadPointer(ch);
        auto* high = highBandBuffer.getReadPointer(ch);
        
        for (int i = 0; i < numSamples; ++i) {
            output[i] = low[i] + high[i];
        }
    }
    
    // === OUTPUT GAIN ===
    juce::dsp::AudioBlock<float> outputBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> outputContext(outputBlock);
    
    // Minimal auto-makeup (let the punch come through)
    float autoMakeup = -driveDb * 0.15f; // Only 15% compensation
    float finalOutputDb = autoMakeup + currentOutput;
    outputGain.setGainDecibels(finalOutputDb);
    outputGain.process(outputContext);
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
