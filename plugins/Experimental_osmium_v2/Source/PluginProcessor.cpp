#include "PluginProcessor.h"
#include "PluginEditor.h"

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

juce::AudioProcessorValueTreeState::ParameterLayout OsmiumAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::intensity, 1),
        "Osmium Core",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.15f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::outputGain, 1),
        "Output",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::bypass, 1),
        "Bypass",
        false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::expManualMode, 1),
        "EXP Manual Mode",
        true));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expCrossoverHz, 1),
        "EXP Crossover (Hz)",
        juce::NormalisableRange<float>(220.0f, 600.0f, 1.0f, 0.4f),
        277.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::expOversamplingMode, 1),
        "EXP Oversampling",
        juce::StringArray { "Off", "4x", "8x" },
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowAttackBoostDb, 1),
        "EXP Low Attack Boost (dB)",
        juce::NormalisableRange<float>(0.0f, 11.9f, 0.1f),
        1.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowAttackTimeMs, 1),
        "EXP Low Attack Time (ms)",
        juce::NormalisableRange<float>(14.0f, 30.8f, 0.1f, 0.45f),
        14.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowPostCompDb, 1),
        "EXP Low Post Attack Comp (dB)",
        juce::NormalisableRange<float>(0.0f, 6.9f, 0.1f),
        0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowCompTimeMs, 1),
        "EXP Low Compression Time (ms)",
        juce::NormalisableRange<float>(24.0f, 68.5f, 0.1f, 0.4f),
        24.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowBodyLiftDb, 1),
        "EXP Low Body Lift (dB)",
        juce::NormalisableRange<float>(0.0f, 11.9f, 0.1f),
        0.6f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowSatMix, 1),
        "EXP Low Saturation Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.13f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowSatDrive, 1),
        "EXP Low Saturation Drive",
        juce::NormalisableRange<float>(1.0f, 5.0f, 0.01f),
        1.30f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::expLowWaveShaperType, 1),
        "EXP Low Waveshaper",
        juce::StringArray { "Soft Sign", "Sine", "Hard Clip" },
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighAttackBoostDb, 1),
        "EXP High Attack Boost (dB)",
        juce::NormalisableRange<float>(0.0f, 15.9f, 0.1f),
        1.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighAttackTimeMs, 1),
        "EXP High Attack Time (ms)",
        juce::NormalisableRange<float>(6.0f, 12.7f, 0.1f, 0.45f),
        6.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighPostCompDb, 1),
        "EXP High Post Attack Comp (dB)",
        juce::NormalisableRange<float>(0.0f, 7.4f, 0.1f),
        0.4f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighCompTimeMs, 1),
        "EXP High Compression Time (ms)",
        juce::NormalisableRange<float>(14.0f, 38.1f, 0.1f, 0.4f),
        14.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighDriveDb, 1),
        "EXP High Drive (dB)",
        juce::NormalisableRange<float>(0.0f, 9.5f, 0.1f),
        0.6f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighSatMix, 1),
        "EXP High Saturation Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.05f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighSatDrive, 1),
        "EXP High Saturation Drive",
        juce::NormalisableRange<float>(1.0f, 5.0f, 0.01f),
        1.24f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighTrimDb, 1),
        "EXP High Band Trim (dB)",
        juce::NormalisableRange<float>(-4.0f, -0.1f, 0.1f),
        -0.1f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::expHighWaveShaperType, 1),
        "EXP High Waveshaper",
        juce::StringArray { "Soft Sign", "Sine", "Hard Clip" },
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightSustainDepthDb, 1),
        "EXP Tight Sustain Depth (dB)",
        juce::NormalisableRange<float>(0.0f, 18.0f, 0.1f),
        0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightReleaseMs, 1),
        "EXP Tight Release (ms)",
        juce::NormalisableRange<float>(20.0f, 220.0f, 0.1f, 0.45f),
        90.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightBellFreqHz, 1),
        "EXP Tight Bell Frequency (Hz)",
        juce::NormalisableRange<float>(200.0f, 600.0f, 1.0f, 0.5f),
        360.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightBellCutDb, 1),
        "EXP Tight Bell Cut (dB)",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f),
        0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightBellQ, 1),
        "EXP Tight Bell Q",
        juce::NormalisableRange<float>(0.4f, 1.8f, 0.01f, 0.45f),
        0.90f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightHighShelfCutDb, 1),
        "EXP Tight High Shelf Cut (dB)",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f),
        0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightHighShelfFreqHz, 1),
        "EXP Tight High Shelf Frequency (Hz)",
        juce::NormalisableRange<float>(2500.0f, 12000.0f, 10.0f, 0.5f),
        6000.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLimiterThresholdDb, 1),
        "EXP Limiter Threshold (dB)",
        juce::NormalisableRange<float>(-0.1f, -0.1f, 0.1f),
        -0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLimiterReleaseMs, 1),
        "EXP Limiter Release (ms)",
        juce::NormalisableRange<float>(80.0f, 80.0f, 0.1f, 0.45f),
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

void OsmiumAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
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

    outputGain.prepare(spec);
    outputLimiter.prepare(spec);
    outputLimiter.setThreshold(-0.1f);
    outputLimiter.setRelease(80.0f);

    lowBandBuffer.setSize(numChannels, samplesPerBlock);
    highBandBuffer.setSize(numChannels, samplesPerBlock);

    lowOversampling4x = std::make_unique<juce::dsp::Oversampling<float>>(
        static_cast<size_t>(numChannels),
        2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        false);
    lowOversampling8x = std::make_unique<juce::dsp::Oversampling<float>>(
        static_cast<size_t>(numChannels),
        3,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        false);

    highOversampling4x = std::make_unique<juce::dsp::Oversampling<float>>(
        static_cast<size_t>(numChannels),
        2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        false);
    highOversampling8x = std::make_unique<juce::dsp::Oversampling<float>>(
        static_cast<size_t>(numChannels),
        3,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        false);

    lowOversampling4x->reset();
    lowOversampling8x->reset();
    highOversampling4x->reset();
    highOversampling8x->reset();

    lowOversampling4x->initProcessing(static_cast<size_t>(samplesPerBlock));
    lowOversampling8x->initProcessing(static_cast<size_t>(samplesPerBlock));
    highOversampling4x->initProcessing(static_cast<size_t>(samplesPerBlock));
    highOversampling8x->initProcessing(static_cast<size_t>(samplesPerBlock));

    auto prepareFilter = [numChannels](juce::dsp::FirstOrderTPTFilter<float>& filter,
                                       double sr,
                                       int maxBlockSize,
                                       juce::dsp::FirstOrderTPTFilterType type,
                                       float cutoffHz)
    {
        juce::dsp::ProcessSpec fSpec;
        fSpec.sampleRate = sr;
        fSpec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        fSpec.numChannels = static_cast<juce::uint32>(numChannels);

        filter.prepare(fSpec);
        filter.setType(type);
        filter.setCutoffFrequency(cutoffHz);
        filter.reset();
    };

    const float postCutoffOff = juce::jmin(20000.0f, static_cast<float>(sampleRate * 0.45));
    const float postCutoff4x = juce::jmin(20000.0f, static_cast<float>(sampleRate * 4.0 * 0.45));
    const float postCutoff8x = juce::jmin(20000.0f, static_cast<float>(sampleRate * 8.0 * 0.45));

    prepareFilter(lowPreFilterOff, sampleRate, samplesPerBlock, juce::dsp::FirstOrderTPTFilterType::highpass, 20.0f);
    prepareFilter(lowPostFilterOff, sampleRate, samplesPerBlock, juce::dsp::FirstOrderTPTFilterType::lowpass, postCutoffOff);
    prepareFilter(highPreFilterOff, sampleRate, samplesPerBlock, juce::dsp::FirstOrderTPTFilterType::highpass, 20.0f);
    prepareFilter(highPostFilterOff, sampleRate, samplesPerBlock, juce::dsp::FirstOrderTPTFilterType::lowpass, postCutoffOff);

    prepareFilter(lowPreFilter4x, sampleRate * 4.0, samplesPerBlock * 4, juce::dsp::FirstOrderTPTFilterType::highpass, 20.0f);
    prepareFilter(lowPostFilter4x, sampleRate * 4.0, samplesPerBlock * 4, juce::dsp::FirstOrderTPTFilterType::lowpass, postCutoff4x);
    prepareFilter(highPreFilter4x, sampleRate * 4.0, samplesPerBlock * 4, juce::dsp::FirstOrderTPTFilterType::highpass, 20.0f);
    prepareFilter(highPostFilter4x, sampleRate * 4.0, samplesPerBlock * 4, juce::dsp::FirstOrderTPTFilterType::lowpass, postCutoff4x);

    prepareFilter(lowPreFilter8x, sampleRate * 8.0, samplesPerBlock * 8, juce::dsp::FirstOrderTPTFilterType::highpass, 20.0f);
    prepareFilter(lowPostFilter8x, sampleRate * 8.0, samplesPerBlock * 8, juce::dsp::FirstOrderTPTFilterType::lowpass, postCutoff8x);
    prepareFilter(highPreFilter8x, sampleRate * 8.0, samplesPerBlock * 8, juce::dsp::FirstOrderTPTFilterType::highpass, 20.0f);
    prepareFilter(highPostFilter8x, sampleRate * 8.0, samplesPerBlock * 8, juce::dsp::FirstOrderTPTFilterType::lowpass, postCutoff8x);

    tightBellFilter.prepare(spec);
    tightHighShelfFilter.prepare(spec);
    *tightBellFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 360.0f, 0.90f, 1.0f);
    *tightHighShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 6000.0f, 0.7071f, 1.0f);

    tightFastEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
    tightSlowEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
    tightProgramEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
    tightGainDbEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);

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

float OsmiumAudioProcessor::applyWaveShaper(float input, WaveShaperType type) const
{
    switch (type)
    {
        case WaveShaperType::softSign:
            return input / (1.0f + std::abs(input));
        case WaveShaperType::sine:
            return std::sin(juce::jlimit(-1.0f, 1.0f, input) * juce::MathConstants<float>::halfPi);
        case WaveShaperType::hardClip:
            return juce::jlimit(-1.0f, 1.0f, input);
        default:
            return input / (1.0f + std::abs(input));
    }
}

void OsmiumAudioProcessor::applySaturationStage(juce::AudioBuffer<float>& bandBuffer,
                                                float mix,
                                                float drive,
                                                int oversamplingMode,
                                                WaveShaperType waveShaperType,
                                                SaturationBand band)
{
    mix = juce::jlimit(0.0f, 1.0f, mix);
    drive = juce::jmax(0.0f, drive);

    if (mix <= 0.0f)
        return;

    const auto processShaper = [this, mix, drive, waveShaperType](juce::dsp::AudioBlock<float>& block)
    {
        const auto numChannels = block.getNumChannels();
        const auto numSamples = block.getNumSamples();

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* data = block.getChannelPointer(ch);
            for (size_t i = 0; i < numSamples; ++i)
            {
                const float dry = data[i];
                const float shaped = applyWaveShaper(dry * drive, waveShaperType);
                data[i] = dry + (shaped - dry) * mix;
            }
        }
    };

    auto& preOff = (band == SaturationBand::low) ? lowPreFilterOff : highPreFilterOff;
    auto& postOff = (band == SaturationBand::low) ? lowPostFilterOff : highPostFilterOff;
    auto& pre4x = (band == SaturationBand::low) ? lowPreFilter4x : highPreFilter4x;
    auto& post4x = (band == SaturationBand::low) ? lowPostFilter4x : highPostFilter4x;
    auto& pre8x = (band == SaturationBand::low) ? lowPreFilter8x : highPreFilter8x;
    auto& post8x = (band == SaturationBand::low) ? lowPostFilter8x : highPostFilter8x;

    const float baseSr = static_cast<float>(getSampleRate());
    const float cutoffOff = juce::jmin(20000.0f, baseSr * 0.45f);
    const float cutoff4x = juce::jmin(20000.0f, baseSr * 4.0f * 0.45f);
    const float cutoff8x = juce::jmin(20000.0f, baseSr * 8.0f * 0.45f);
    postOff.setCutoffFrequency(cutoffOff);
    post4x.setCutoffFrequency(cutoff4x);
    post8x.setCutoffFrequency(cutoff8x);

    auto* over4x = (band == SaturationBand::low) ? lowOversampling4x.get() : highOversampling4x.get();
    auto* over8x = (band == SaturationBand::low) ? lowOversampling8x.get() : highOversampling8x.get();

    juce::dsp::AudioBlock<float> baseBlock(bandBuffer);

    if (oversamplingMode == 1 && over4x != nullptr)
    {
        auto upBlock = over4x->processSamplesUp(baseBlock);
        juce::dsp::ProcessContextReplacing<float> upContext(upBlock);

        pre4x.process(upContext);
        processShaper(upBlock);
        post4x.process(upContext);

        over4x->processSamplesDown(baseBlock);
        return;
    }

    if (oversamplingMode == 2 && over8x != nullptr)
    {
        auto upBlock = over8x->processSamplesUp(baseBlock);
        juce::dsp::ProcessContextReplacing<float> upContext(upBlock);

        pre8x.process(upContext);
        processShaper(upBlock);
        post8x.process(upContext);

        over8x->processSamplesDown(baseBlock);
        return;
    }

    juce::dsp::ProcessContextReplacing<float> baseContext(baseBlock);
    preOff.process(baseContext);
    processShaper(baseBlock);
    postOff.process(baseContext);
}

void OsmiumAudioProcessor::applyTightnessStage(juce::AudioBuffer<float>& buffer,
                                               float sustainDepthDb,
                                               float sustainReleaseMs,
                                               float bellFreqHz,
                                               float bellCutDb,
                                               float bellQ,
                                               float highShelfCutDb,
                                               float highShelfFreqHz)
{
    const auto sr = static_cast<float>(getSampleRate());
    if (sr <= 0.0f)
        return;

    sustainDepthDb = juce::jlimit(0.0f, 18.0f, sustainDepthDb);
    sustainReleaseMs = juce::jlimit(20.0f, 220.0f, sustainReleaseMs);
    bellFreqHz = juce::jlimit(200.0f, 600.0f, bellFreqHz);
    bellCutDb = juce::jlimit(0.0f, 12.0f, bellCutDb);
    bellQ = juce::jlimit(0.4f, 1.8f, bellQ);
    highShelfCutDb = juce::jlimit(0.0f, 12.0f, highShelfCutDb);
    highShelfFreqHz = juce::jlimit(2500.0f, juce::jmin(12000.0f, sr * 0.45f), highShelfFreqHz);

    *tightBellFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sr, bellFreqHz, bellQ, juce::Decibels::decibelsToGain(-bellCutDb));
    *tightHighShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sr, highShelfFreqHz, 0.7071f, juce::Decibels::decibelsToGain(-highShelfCutDb));

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (static_cast<int>(tightFastEnvelope.size()) != numChannels)
    {
        tightFastEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
        tightSlowEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
        tightProgramEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
        tightGainDbEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
    }

    const float fastAttackCoeff = std::exp(-1.0f / (sr * 0.001f * 1.5f));
    const float fastReleaseCoeff = std::exp(-1.0f / (sr * 0.001f * 18.0f));
    const float slowAttackCoeff = std::exp(-1.0f / (sr * 0.001f * 12.0f));
    const float slowReleaseCoeff = std::exp(-1.0f / (sr * 0.001f * 180.0f));
    const float programAttackCoeff = std::exp(-1.0f / (sr * 0.001f * 120.0f));
    const float programReleaseCoeff = std::exp(-1.0f / (sr * 0.001f * 1800.0f));
    const float sustainAttackCoeff = std::exp(-1.0f / (sr * 0.001f * 4.0f));
    const float sustainReleaseCoeff = std::exp(-1.0f / (sr * 0.001f * sustainReleaseMs));

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        float fastEnv = tightFastEnvelope[static_cast<size_t>(ch)];
        float slowEnv = tightSlowEnvelope[static_cast<size_t>(ch)];
        float programEnv = tightProgramEnvelope[static_cast<size_t>(ch)];
        float gainDbEnv = tightGainDbEnvelope[static_cast<size_t>(ch)];

        for (int i = 0; i < numSamples; ++i)
        {
            const float in = data[i];
            const float sampleAbs = std::abs(in);

            const float fastCoeff = (sampleAbs > fastEnv) ? fastAttackCoeff : fastReleaseCoeff;
            const float slowCoeff = (sampleAbs > slowEnv) ? slowAttackCoeff : slowReleaseCoeff;
            fastEnv = fastCoeff * fastEnv + (1.0f - fastCoeff) * sampleAbs;
            slowEnv = slowCoeff * slowEnv + (1.0f - slowCoeff) * sampleAbs;
            const float programCoeff = (sampleAbs > programEnv) ? programAttackCoeff : programReleaseCoeff;
            programEnv = programCoeff * programEnv + (1.0f - programCoeff) * sampleAbs;

            const float transientAmount = juce::jlimit(0.0f, 1.0f, ((fastEnv - slowEnv) / (slowEnv + 1.0e-4f)) * 2.0f);
            const float sustainAmount = 1.0f - transientAmount;
            const float levelDb = juce::Decibels::gainToDecibels(slowEnv + 1.0e-6f, -100.0f);
            const float programDb = juce::Decibels::gainToDecibels(programEnv + 1.0e-6f, -100.0f);
            const float autoThresholdDb = juce::jlimit(-72.0f, -6.0f, programDb - 16.0f);
            const float thresholdGate = juce::jlimit(0.0f, 1.0f, (levelDb - autoThresholdDb) / 18.0f);
            const float targetGainDb = -sustainDepthDb * sustainAmount * thresholdGate;

            const float gainCoeff = (targetGainDb < gainDbEnv) ? sustainAttackCoeff : sustainReleaseCoeff;
            gainDbEnv = gainCoeff * gainDbEnv + (1.0f - gainCoeff) * targetGainDb;

            data[i] = in * juce::Decibels::decibelsToGain(gainDbEnv);
        }

        tightFastEnvelope[static_cast<size_t>(ch)] = fastEnv;
        tightSlowEnvelope[static_cast<size_t>(ch)] = slowEnv;
        tightProgramEnvelope[static_cast<size_t>(ch)] = programEnv;
        tightGainDbEnvelope[static_cast<size_t>(ch)] = gainDbEnv;
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    tightBellFilter.process(context);
    tightHighShelfFilter.process(context);
}

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
    const bool manualMode = true;
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

    float crossoverHz = juce::jmap(intensity, 220.0f, 600.0f);

    float lowAttackBoost = juce::jmap(intensity, 0.0f, 7.3563636f) + density * 4.5436364f;
    float lowPostAttackComp = juce::jmap(intensity, 0.0f, 3.01875f) + density * 3.88125f;
    float lowAttackTimeMs = juce::jmap(density, 14.0f, 30.8f);
    float lowCompTimeMs = juce::jmap(density, 24.0f, 68.5f);
    float lowBodyLiftDb = juce::jmap(intensity, 0.0f, 3.9666667f) + density * 7.9333333f;
    const float lowSatSkewed = std::pow(intensity, 0.6f);
    float lowSatMix = juce::jmap(lowSatSkewed, 0.0f, 0.39f);
    float lowSatDrive = juce::jmap(lowSatSkewed, 1.0f, 1.94f);

    float highAttackBoost = juce::jmap(intensity, 0.0f, 8.5615385f) + density * 7.3384615f;
    float highPostAttackComp = juce::jmap(intensity, 0.0f, 2.3368421f) + density * 5.0631579f;
    float highAttackTimeMs = juce::jmap(density, 6.0f, 12.7f);
    float highCompTimeMs = juce::jmap(density, 14.0f, 38.1f);
    float highDriveDb = juce::jmap(intensity, 0.0f, 3.99f) + density * 5.51f;
    const float highSatSkewed = std::pow(intensity, 0.6f);
    float highSatMix = juce::jlimit(0.0f, 0.46f, juce::jmap(highSatSkewed, 0.0f, 0.1686667f) + density * 0.2913333f);
    float highSatDrive = juce::jmap(highSatSkewed, 1.0f, 1.7392f) + density * 0.9408f;
    const float highTrimSkewed = std::pow(intensity, 1.7f);
    float highBandTrimDb = -(juce::jmap(highTrimSkewed, 0.1f, 0.8090909f) + density * 3.1909091f);

    float limiterThresholdDb = -0.1f;
    float limiterReleaseMs = 80.0f;
    float autoMakeupDb = -highDriveDb * (0.10f + density * 0.08f);
    float bodyHoldDb = juce::jmap(intensity, 0.0f, 0.2f) + density * 0.8f;

    int oversamplingMode = 0;
    auto lowWaveShaperType = WaveShaperType::softSign;
    auto highWaveShaperType = WaveShaperType::softSign;

    float tightSustainDepthDb = 0.0f;
    float tightReleaseMs = 90.0f;
    float tightBellFreqHz = 360.0f;
    float tightBellCutDb = 0.0f;
    float tightBellQ = 0.90f;
    float tightHighShelfCutDb = 0.0f;
    float tightHighShelfFreqHz = 6000.0f;

    if (manualMode)
    {
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

        oversamplingMode = juce::jlimit(0, 2, juce::roundToInt(getParam(ParameterIDs::expOversamplingMode)));
        lowWaveShaperType = static_cast<WaveShaperType>(juce::jlimit(0, 2, juce::roundToInt(getParam(ParameterIDs::expLowWaveShaperType))));
        highWaveShaperType = static_cast<WaveShaperType>(juce::jlimit(0, 2, juce::roundToInt(getParam(ParameterIDs::expHighWaveShaperType))));

        tightSustainDepthDb = getParam(ParameterIDs::expTightSustainDepthDb);
        tightReleaseMs = getParam(ParameterIDs::expTightReleaseMs);
        tightBellFreqHz = getParam(ParameterIDs::expTightBellFreqHz);
        tightBellCutDb = getParam(ParameterIDs::expTightBellCutDb);
        tightBellQ = getParam(ParameterIDs::expTightBellQ);
        tightHighShelfCutDb = getParam(ParameterIDs::expTightHighShelfCutDb);
        tightHighShelfFreqHz = getParam(ParameterIDs::expTightHighShelfFreqHz);

        autoMakeupDb = -highDriveDb * 0.04f;
        bodyHoldDb = 0.0f;
    }

    crossoverHz = juce::jlimit(220.0f, 600.0f, crossoverHz);
    lowSatMix = juce::jlimit(0.0f, 1.0f, lowSatMix);
    lowSatDrive = juce::jlimit(1.0f, 5.0f, lowSatDrive);
    highSatMix = juce::jlimit(0.0f, 1.0f, highSatMix);
    highSatDrive = juce::jlimit(1.0f, 5.0f, highSatDrive);
    highBandTrimDb = juce::jlimit(-4.0f, -0.1f, highBandTrimDb);
    tightSustainDepthDb = juce::jlimit(0.0f, 18.0f, tightSustainDepthDb);
    tightReleaseMs = juce::jlimit(20.0f, 220.0f, tightReleaseMs);
    tightBellFreqHz = juce::jlimit(200.0f, 600.0f, tightBellFreqHz);
    tightBellCutDb = juce::jlimit(0.0f, 12.0f, tightBellCutDb);
    tightBellQ = juce::jlimit(0.4f, 1.8f, tightBellQ);
    tightHighShelfCutDb = juce::jlimit(0.0f, 12.0f, tightHighShelfCutDb);
    tightHighShelfFreqHz = juce::jlimit(2500.0f, 12000.0f, tightHighShelfFreqHz);
    limiterThresholdDb = -0.1f;
    limiterReleaseMs = 80.0f;

    lowpassFilter.setCutoffFrequency(crossoverHz);
    highpassFilter.setCutoffFrequency(crossoverHz);
    outputLimiter.setThreshold(limiterThresholdDb);
    outputLimiter.setRelease(limiterReleaseMs);

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

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

    lowpassFilter.process(lowContext);
    highpassFilter.process(highContext);

    lowBandTransientShaper.setParameters(lowAttackBoost, lowPostAttackComp, lowAttackTimeMs, lowCompTimeMs);
    lowBandTransientShaper.process(lowBandBuffer);

    const float lowBodyLift = juce::Decibels::decibelsToGain(lowBodyLiftDb);
    if (std::abs(lowBodyLift - 1.0f) > 1.0e-5f) {
        for (int ch = 0; ch < numChannels; ++ch)
            lowBandBuffer.applyGain(ch, 0, numSamples, lowBodyLift);
    }

    applySaturationStage(lowBandBuffer, lowSatMix, lowSatDrive, oversamplingMode, lowWaveShaperType, SaturationBand::low);

    lowBandLimiter.process(lowContext);

    highBandTransientShaper.setParameters(highAttackBoost, highPostAttackComp, highAttackTimeMs, highCompTimeMs);
    highBandTransientShaper.process(highBandBuffer);

    highBandDrive.setGainDecibels(highDriveDb);
    highBandDrive.process(highContext);

    applySaturationStage(highBandBuffer, highSatMix, highSatDrive, oversamplingMode, highWaveShaperType, SaturationBand::high);

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

    applyTightnessStage(buffer,
                        tightSustainDepthDb,
                        tightReleaseMs,
                        tightBellFreqHz,
                        tightBellCutDb,
                        tightBellQ,
                        tightHighShelfCutDb,
                        tightHighShelfFreqHz);

    juce::dsp::AudioBlock<float> outputBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> outputContext(outputBlock);

    const float finalOutputDb = autoMakeupDb + bodyHoldDb + currentOutput;
    outputGain.setGainDecibels(finalOutputDb);
    outputGain.process(outputContext);
    outputLimiter.process(outputContext);
}

bool OsmiumAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OsmiumAudioProcessor::createEditor()
{
    return new OsmiumAudioProcessorEditor (*this);
}

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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OsmiumAudioProcessor();
}
