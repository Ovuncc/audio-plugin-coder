#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

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

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::oversamplingMode, 1),
        "Oversampling",
        juce::StringArray { "Off", "4x", "8x" },
        1));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::processingMode, 1),
        "Mode",
        juce::StringArray { "Osmium", "Tight", "Chaotic" },
        0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::tightLookaheadMode, 1),
        "Tight Lookahead",
        juce::StringArray { "Off", "0.5 ms", "1.0 ms", "2.0 ms" },
        1));

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

    outputGain.prepare(spec);

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
    *tightBellFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 356.0f, 0.40f, 1.0f);
    *tightHighShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, juce::jmin(7000.0f, static_cast<float>(sampleRate * 0.45)), 0.7071f, 1.0f);

    tightFastEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
    tightSlowEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
    tightProgramEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
    tightGainDbEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);

    tightLookaheadBufferLength = juce::jmax(1, juce::roundToInt(sampleRate * 0.001 * 2.0) + 1);
    tightLookaheadBuffers.assign(static_cast<size_t>(numChannels), std::vector<float>(static_cast<size_t>(tightLookaheadBufferLength), 0.0f));
    tightLookaheadWritePositions.assign(static_cast<size_t>(numChannels), 0);
    tightLookaheadSamplesCurrent = 0;
    tightReportedLatencySamples = -1;
    setLatencySamples(0);

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
        case WaveShaperType::sine:
            return std::sin(juce::jlimit(-1.0f, 1.0f, input) * juce::MathConstants<float>::halfPi);
        case WaveShaperType::softSign:
            return input / (1.0f + std::abs(input));
        case WaveShaperType::hardClip:
            return juce::jlimit(-1.0f, 1.0f, input);
        default:
            return std::sin(juce::jlimit(-1.0f, 1.0f, input) * juce::MathConstants<float>::halfPi);
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

    if (oversamplingMode == 0)
    {
        juce::dsp::AudioBlock<float> baseBlock(bandBuffer);
        processShaper(baseBlock);
        return;
    }

    auto* over = (oversamplingMode == 1)
        ? ((band == SaturationBand::low) ? lowOversampling4x.get() : highOversampling4x.get())
        : ((band == SaturationBand::low) ? lowOversampling8x.get() : highOversampling8x.get());

    if (over == nullptr)
    {
        juce::dsp::AudioBlock<float> baseBlock(bandBuffer);
        processShaper(baseBlock);
        return;
    }

    auto& pre = (oversamplingMode == 1)
        ? ((band == SaturationBand::low) ? lowPreFilter4x : highPreFilter4x)
        : ((band == SaturationBand::low) ? lowPreFilter8x : highPreFilter8x);
    auto& post = (oversamplingMode == 1)
        ? ((band == SaturationBand::low) ? lowPostFilter4x : highPostFilter4x)
        : ((band == SaturationBand::low) ? lowPostFilter8x : highPostFilter8x);

    const float sr = static_cast<float>(getSampleRate());
    const float factor = (oversamplingMode == 1) ? 4.0f : 8.0f;
    const float postCutoff = juce::jmin(20000.0f, sr * factor * 0.45f);
    post.setCutoffFrequency(postCutoff);

    juce::dsp::AudioBlock<float> baseBlock(bandBuffer);
    auto upBlock = over->processSamplesUp(baseBlock);
    juce::dsp::ProcessContextReplacing<float> upContext(upBlock);

    pre.process(upContext);
    processShaper(upBlock);
    post.process(upContext);

    over->processSamplesDown(baseBlock);
}

void OsmiumAudioProcessor::applyTightnessStage(juce::AudioBuffer<float>& buffer,
                                               float sustainDepthDb,
                                               float sustainReleaseMs,
                                               float bellFreqHz,
                                               float bellCutDb,
                                               float bellQ,
                                               float highShelfCutDb,
                                               float highShelfFreqHz,
                                               int lookaheadSamples)
{
    const auto sr = static_cast<float>(getSampleRate());
    if (sr <= 0.0f)
        return;

    sustainDepthDb = juce::jlimit(0.0f, 18.0f, sustainDepthDb);
    sustainReleaseMs = juce::jmax(1.0f, sustainReleaseMs);
    bellFreqHz = juce::jlimit(200.0f, 600.0f, bellFreqHz);
    bellCutDb = juce::jlimit(0.0f, 5.0f, bellCutDb);
    bellQ = juce::jlimit(0.2f, 2.0f, bellQ);
    highShelfCutDb = juce::jlimit(0.0f, 2.5f, highShelfCutDb);
    highShelfFreqHz = juce::jlimit(3500.0f, juce::jmin(12000.0f, sr * 0.45f), highShelfFreqHz);

    if (tightLookaheadBufferLength <= 0)
        tightLookaheadBufferLength = juce::jmax(1, juce::roundToInt(sr * 0.001f * 2.0f) + 1);
    lookaheadSamples = juce::jlimit(0, tightLookaheadBufferLength - 1, lookaheadSamples);

    // Exact zero-depth/cut should be fully transparent in Tight mode.
    if (sustainDepthDb <= 1.0e-4f && bellCutDb <= 1.0e-4f && highShelfCutDb <= 1.0e-4f)
    {
        for (auto& gainDbEnv : tightGainDbEnvelope)
            gainDbEnv = 0.0f;
        return;
    }

    *tightBellFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sr, bellFreqHz, bellQ, juce::Decibels::decibelsToGain(-bellCutDb));
    *tightHighShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sr, highShelfFreqHz, 0.71f, juce::Decibels::decibelsToGain(-highShelfCutDb));

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (static_cast<int>(tightFastEnvelope.size()) != numChannels)
    {
        tightFastEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
        tightSlowEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
        tightProgramEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
        tightGainDbEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
        tightLookaheadBuffers.assign(static_cast<size_t>(numChannels), std::vector<float>(static_cast<size_t>(tightLookaheadBufferLength), 0.0f));
        tightLookaheadWritePositions.assign(static_cast<size_t>(numChannels), 0);
    }

    if (lookaheadSamples != tightLookaheadSamplesCurrent)
    {
        for (auto& channelBuffer : tightLookaheadBuffers)
            std::fill(channelBuffer.begin(), channelBuffer.end(), 0.0f);
        std::fill(tightLookaheadWritePositions.begin(), tightLookaheadWritePositions.end(), 0);
        tightLookaheadSamplesCurrent = lookaheadSamples;
    }

    const float fastAttackCoeff = std::exp(-1.0f / (sr * 0.001f * 0.10f));
    const float fastReleaseCoeff = std::exp(-1.0f / (sr * 0.001f * 84.68f));
    const float slowAttackCoeff = std::exp(-1.0f / (sr * 0.001f * 24.29f));
    const float slowReleaseCoeff = std::exp(-1.0f / (sr * 0.001f * 600.0f));
    const float programAttackCoeff = std::exp(-1.0f / (sr * 0.001f * 80.40f));
    const float programReleaseCoeff = std::exp(-1.0f / (sr * 0.001f * 100.0f));
    const float sustainAttackCoeff = std::exp(-1.0f / (sr * 0.001f * 30.0f));
    const float sustainReleaseCoeff = std::exp(-1.0f / (sr * 0.001f * sustainReleaseMs));

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        float fastEnv = tightFastEnvelope[static_cast<size_t>(ch)];
        float slowEnv = tightSlowEnvelope[static_cast<size_t>(ch)];
        float programEnv = tightProgramEnvelope[static_cast<size_t>(ch)];
        float gainDbEnv = tightGainDbEnvelope[static_cast<size_t>(ch)];
        int lookaheadWritePosition = 0;

        if (!tightLookaheadWritePositions.empty())
            lookaheadWritePosition = tightLookaheadWritePositions[static_cast<size_t>(ch)];

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

            const float transientAmount = juce::jlimit(0.0f, 1.0f, ((fastEnv - slowEnv) / (slowEnv + 1.0e-4f)) * 2.8f);
            const float sustainAmountBase = 1.0f - transientAmount;
            const float sustainAmount = std::pow(sustainAmountBase, 1.7f);
            const float levelDb = juce::Decibels::gainToDecibels(slowEnv + 1.0e-6f, -100.0f);
            const float programDb = juce::Decibels::gainToDecibels(programEnv + 1.0e-6f, -100.0f);
            const float autoThresholdDb = juce::jlimit(-72.0f, -11.1f, programDb - 16.4f);
            const float thresholdGate = juce::jlimit(0.0f, 1.0f, (levelDb - autoThresholdDb) / 17.9f);
            const float smoothedGate = thresholdGate * thresholdGate;
            const float targetGainDb = -sustainDepthDb * sustainAmount * smoothedGate;

            const float gainCoeff = (targetGainDb < gainDbEnv) ? sustainAttackCoeff : sustainReleaseCoeff;
            gainDbEnv = gainCoeff * gainDbEnv + (1.0f - gainCoeff) * targetGainDb;

            const float dynamicGain = juce::Decibels::decibelsToGain(gainDbEnv);

            if (lookaheadSamples > 0 && !tightLookaheadBuffers.empty())
            {
                auto& lookaheadChannel = tightLookaheadBuffers[static_cast<size_t>(ch)];
                const int channelBufferLength = static_cast<int>(lookaheadChannel.size());
                lookaheadChannel[static_cast<size_t>(lookaheadWritePosition)] = in;

                int readPosition = lookaheadWritePosition - lookaheadSamples;
                if (readPosition < 0)
                    readPosition += channelBufferLength;
                const float delayed = lookaheadChannel[static_cast<size_t>(readPosition)];

                data[i] = delayed * dynamicGain;

                ++lookaheadWritePosition;
                if (lookaheadWritePosition >= channelBufferLength)
                    lookaheadWritePosition = 0;
            }
            else
            {
                data[i] = in * dynamicGain;
            }
        }

        tightFastEnvelope[static_cast<size_t>(ch)] = fastEnv;
        tightSlowEnvelope[static_cast<size_t>(ch)] = slowEnv;
        tightProgramEnvelope[static_cast<size_t>(ch)] = programEnv;
        tightGainDbEnvelope[static_cast<size_t>(ch)] = gainDbEnv;

        if (!tightLookaheadWritePositions.empty())
            tightLookaheadWritePositions[static_cast<size_t>(ch)] = lookaheadWritePosition;
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    tightBellFilter.process(context);
    tightHighShelfFilter.process(context);
}

void OsmiumAudioProcessor::applyOutputSoftClipper(juce::AudioBuffer<float>& buffer,
                                                  float ceilingDb,
                                                  float drive) const
{
    ceilingDb = juce::jlimit(-0.6f, 0.0f, ceilingDb);
    drive = juce::jlimit(1.0f, 4.0f, drive);

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    const float ceilingGain = juce::Decibels::decibelsToGain(ceilingDb);
    const float invSoftClipNorm = 1.0f / std::tanh(drive);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float normalized = data[i] / ceilingGain;
            const float clipped = std::tanh(normalized * drive) * invSoftClipNorm;
            data[i] = clipped * ceilingGain;
        }
    }
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
    const int oversamplingMode = juce::jlimit(0, 2, juce::roundToInt(getParam(ParameterIDs::oversamplingMode)));
    const auto mode = static_cast<ProcessingMode>(juce::jlimit(0, 2, juce::roundToInt(getParam(ParameterIDs::processingMode))));
    const int tightLookaheadMode = juce::jlimit(0, 3, juce::roundToInt(getParam(ParameterIDs::tightLookaheadMode)));

    smoothedIntensity.setTargetValue(intensityTarget);
    smoothedOutput.setTargetValue(outputTarget);
    smoothedIntensity.skip(buffer.getNumSamples());
    smoothedOutput.skip(buffer.getNumSamples());

    const float currentIntensity = smoothedIntensity.getCurrentValue();
    const float currentOutput = smoothedOutput.getCurrentValue();
    const float intensity = juce::jlimit(0.0f, 1.0f, currentIntensity);
    const float densityRegion = juce::jlimit(0.0f, 1.0f, (intensity - 0.60f) / 0.40f);
    const float density = densityRegion * densityRegion;

    const float crossoverHz = juce::jmap(intensity, 220.0f, 600.0f);

    float lowAttackBoost = juce::jmap(intensity, 0.0f, 7.3563636f) + density * 4.5436364f;
    float lowPostAttackComp = juce::jmap(intensity, 0.0f, 3.01875f) + density * 3.88125f;
    float lowAttackTimeMs = juce::jmap(density, 14.0f, 30.8f);
    float lowCompTimeMs = juce::jmap(density, 24.0f, 68.5f);
    float lowBodyLiftDb = juce::jmap(intensity, 0.0f, 3.9666667f) + density * 7.9333333f;
    const float lowSatSkewed = std::pow(intensity, 0.6f);
    float lowSatMix = juce::jmap(lowSatSkewed, 0.0f, 1.0f);

    float highAttackBoost = juce::jmap(intensity, 0.0f, 8.5615385f) + density * 7.3384615f;
    float highPostAttackComp = juce::jmap(intensity, 0.0f, 2.3368421f) + density * 5.0631579f;
    float highAttackTimeMs = juce::jmap(density, 6.0f, 12.7f);
    float highCompTimeMs = juce::jmap(density, 14.0f, 38.1f);
    const float highSatSkewed = std::pow(intensity, 0.6f);
    float highSatMix = juce::jmap(highSatSkewed, 0.0f, 1.0f);

    float lowSatDrive = juce::jmap(lowSatSkewed, 1.0f, 1.94f);
    float highSatDrive = juce::jmap(highSatSkewed, 4.0f, 4.7392f) + density * 0.9408f;

    WaveShaperType lowShape = WaveShaperType::sine;
    WaveShaperType highShape = WaveShaperType::sine;

    if (mode == ProcessingMode::tight)
    {
        lowShape = WaveShaperType::hardClip;
        highShape = WaveShaperType::hardClip;

        lowAttackBoost = juce::jmap(intensity, 0.0f, 2.5345454f) + density * 1.5654546f;
        lowPostAttackComp = juce::jmap(intensity, 0.0f, 2.975f) + density * 3.825f;
        lowAttackTimeMs = 30.8f;
        lowCompTimeMs = juce::jmap(density, 24.0f, 68.4f);
        lowBodyLiftDb = juce::jmap(intensity, 0.0f, 3.9666667f) + density * 7.9333333f;
        lowSatMix = 0.0f;
        lowSatDrive = 1.0f;

        highAttackBoost = juce::jmap(intensity, 0.0f, 8.6153846f) + density * 7.3846154f;
        highPostAttackComp = juce::jmap(intensity, 0.0f, 2.3684211f) + density * 5.1315789f;
        highAttackTimeMs = juce::jmap(density, 6.0f, 12.7f);
        highCompTimeMs = juce::jmap(density, 14.0f, 38.1f);
        highSatMix = juce::jmap(highSatSkewed, 0.0f, 0.27f);
        highSatDrive = 2.92f;
    }
    else if (mode == ProcessingMode::chaotic)
    {
        lowShape = WaveShaperType::hardClip;
        highShape = WaveShaperType::hardClip;
        lowSatDrive = juce::jmap(lowSatSkewed, 1.0f, 6.0f);
        highSatDrive = juce::jmap(highSatSkewed, 4.0f, 9.0f);
    }

    lowpassFilter.setCutoffFrequency(crossoverHz);
    highpassFilter.setCutoffFrequency(crossoverHz);

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    lowBandBuffer.setSize(numChannels, numSamples, false, false, true);
    highBandBuffer.setSize(numChannels, numSamples, false, false, true);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        lowBandBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
        highBandBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    juce::dsp::AudioBlock<float> lowBlock(lowBandBuffer);
    juce::dsp::AudioBlock<float> highBlock(highBandBuffer);
    juce::dsp::ProcessContextReplacing<float> lowContext(lowBlock);
    juce::dsp::ProcessContextReplacing<float> highContext(highBlock);

    lowpassFilter.process(lowContext);
    highpassFilter.process(highContext);

    if (mode != ProcessingMode::tight)
    {
        lowBandTransientShaper.setParameters(lowAttackBoost, lowPostAttackComp, lowAttackTimeMs, lowCompTimeMs);
        lowBandTransientShaper.process(lowBandBuffer);
    }

    const float lowBodyLift = juce::Decibels::decibelsToGain(lowBodyLiftDb);
    if (lowBodyLift > 1.0f)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            lowBandBuffer.applyGain(ch, 0, numSamples, lowBodyLift);
    }

    if (mode != ProcessingMode::tight)
        applySaturationStage(lowBandBuffer, lowSatMix, lowSatDrive, oversamplingMode, lowShape, SaturationBand::low);

    if (mode != ProcessingMode::tight)
    {
        lowBandLimiter.setThreshold(-0.15f);
        lowBandLimiter.setRelease(180.0f);
        lowBandLimiter.process(lowContext);
    }

    if (mode == ProcessingMode::tight)
    {
        highBandTransientShaper.setParameters(highAttackBoost,
                                              highPostAttackComp,
                                              highAttackTimeMs,
                                              highCompTimeMs,
                                              0.68f,
                                              10.46f,
                                              20.0f,
                                              5.07f,
                                              0.05f,
                                              0.10f,
                                              juce::jmap(intensity, 2.0f, 6.0f));
    }
    else
    {
        highBandTransientShaper.setParameters(highAttackBoost, highPostAttackComp, highAttackTimeMs, highCompTimeMs);
    }
    highBandTransientShaper.process(highBandBuffer);

    applySaturationStage(highBandBuffer, highSatMix, highSatDrive, oversamplingMode, highShape, SaturationBand::high);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* output = buffer.getWritePointer(ch);
        auto* low = lowBandBuffer.getReadPointer(ch);
        auto* high = highBandBuffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
            output[i] = low[i] + high[i];
    }

    int tightLookaheadSamples = 0;

    if (mode == ProcessingMode::tight)
    {
        const float sustainDepthDb = juce::jmap(intensity, 0.0f, 6.5f);
        const float bellCutDb = 0.0f;
        const float highShelfCutDb = 0.0f;
        const float lookaheadMs = (tightLookaheadMode == 1) ? 0.5f
            : (tightLookaheadMode == 2) ? 1.0f
            : (tightLookaheadMode == 3) ? 2.0f
            : 0.0f;
        tightLookaheadSamples = juce::roundToInt(static_cast<float>(getSampleRate()) * 0.001f * lookaheadMs);
        applyTightnessStage(buffer,
                            sustainDepthDb,
                            46.0f,
                            356.0f,
                            bellCutDb,
                            0.40f,
                            highShelfCutDb,
                            7000.0f,
                            tightLookaheadSamples);
    }

    const int desiredLatencySamples = (mode == ProcessingMode::tight) ? tightLookaheadSamples : 0;
    if (desiredLatencySamples != tightReportedLatencySamples)
    {
        setLatencySamples(desiredLatencySamples);
        tightReportedLatencySamples = desiredLatencySamples;
    }

    const float autoMakeupDb = (mode == ProcessingMode::tight)
        ? -3.8f
        : -(highSatMix * 2.4f + lowSatMix * 1.4f);
    const float bodyHoldDb = (mode == ProcessingMode::tight)
        ? (juce::jmap(intensity, 0.0f, 0.68f) + density * 2.72f)
        : (juce::jmap(intensity, 0.0f, 0.2f) + density * 0.8f);
    const float chaoticPreClipDb = (mode == ProcessingMode::chaotic) ? juce::jmap(intensity, 1.0f, 12.0f) : 0.0f;
    const float preClipGainDb = autoMakeupDb + bodyHoldDb + chaoticPreClipDb;
    if (std::abs(preClipGainDb) > 1.0e-5f)
        buffer.applyGain(juce::Decibels::decibelsToGain(preClipGainDb));

    const float clipperDrive = (mode == ProcessingMode::chaotic) ? juce::jmap(intensity, 2.2f, 3.8f) : 1.85f;
    applyOutputSoftClipper(buffer, -0.10f, clipperDrive);

    juce::dsp::AudioBlock<float> outputBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> outputContext(outputBlock);
    outputGain.setGainDecibels(currentOutput);
    outputGain.process(outputContext);
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
