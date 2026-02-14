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
    // Initialize saturator function (ArcTan/Tanh for tube-like warmth)
    saturator.functionToUse = [](float x) {
        return std::tanh(x * 1.5f); // Soft clipping
    };
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

    // Prepare DSP
    inputGain.prepare(spec);
    
    // Compressor as Transient Shaper (Slow attack lets transients through, Ratio compresses sustain)
    compressor.prepare(spec);
    compressor.setAttack(30.0f); // 30ms attack lets "punch" through
    compressor.setRelease(100.0f);
    compressor.setRatio(4.0f); 

    saturator.prepare(spec);
    
    limiter.prepare(spec);
    limiter.setRelease(100.0f); // Fast release for loudness

    outputGain.prepare(spec);
    
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

    // Skip all smoothing samples to get to target faster
    smoothedIntensity.skip(buffer.getNumSamples());
    smoothedOutput.skip(buffer.getNumSamples());
    
    float currentIntensity = smoothedIntensity.getCurrentValue();
    float currentOutput = smoothedOutput.getCurrentValue();

    // MACRO MAPPING
    // 1. Transient Shaper (Compressor Threshold lowered to engage)
    // At Int=0: Thresh=0dB (No comp). At Int=1: Thresh=-30dB (more aggressive)
    float compThresh = juce::jmap(currentIntensity, 0.0f, -30.0f);
    compressor.setThreshold(compThresh);
    compressor.setRatio(2.0f + (currentIntensity * 6.0f)); // Ratio: 2:1 to 8:1
    
    // 2. Tube Drive (Input Gain into Saturator)
    // Drive: 0dB to +18dB (increased from +12dB for more punch)
    float driveDb = juce::jmap(currentIntensity, 0.0f, 18.0f);
    float driveGain = juce::Decibels::decibelsToGain(driveDb);
    
    // 3. Limiter Threshold
    // Ceiling: -0.1 to -0.5 dB (less aggressive limiting for more loudness)
    float limitThresh = juce::jmap(currentIntensity, -0.1f, -0.5f);
    limiter.setThreshold(limitThresh);

    // DSP CHAIN
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // A. Input Drive (Tube Saturation Pre-Gain)
    inputGain.setGainLinear(driveGain); 
    inputGain.process(context);
    
    // B. Saturator (Soft Clip)
    saturator.process(context);
    
    // C. Transient Shaping (Compressor)
    // Note: To emphasize transients, we often compress the BODY, so the initial attack pops out more relative to sustain.
    // We modify gain reduction based on envelope. 
    compressor.process(context);

    // D. Limiter (Safety)
    limiter.process(context);
    
    // E. Output Compensation
    // Auto-makeup for drive + Manual Output
    // Reduced compensation to allow more perceived loudness increase
    float autoMakeup = -driveDb * 0.25f; // Compensate only 25% of drive (was 50%)
    float finalOutputDb = autoMakeup + currentOutput;
    outputGain.setGainDecibels(finalOutputDb);
    outputGain.process(context);
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
