#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <complex>

MorphantAudioProcessor::MorphantAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
    #if ! JucePlugin_IsMidiEffect
     #if ! JucePlugin_IsSynth
        .withInput("Sound Source", juce::AudioChannelSet::stereo(), true)
        .withInput("Texture Source", juce::AudioChannelSet::stereo(), false)
     #endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    #endif
    )
#endif
    , apvts_(*this, nullptr, "MorphantParameters", createParameterLayout())
{
}

MorphantAudioProcessor::~MorphantAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout MorphantAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto percentText = [](float value, int) { return juce::String(juce::roundToInt(value * 100.0f)) + "%"; };

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::mode, 1),
        "Mode",
        juce::StringArray{ "Filterbank", "Spectral" },
        0
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::debugMode, 1),
        "Debug Mode",
        juce::StringArray{ "Off", "On" },
        0
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::merge, 1),
        "Merge",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.90f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::glue, 1),
        "Glue",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.35f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::pitchFollower, 1),
        "Pitch Follower",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.70f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::formantFollower, 1),
        "Formant Follower",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.55f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::focus, 1),
        "Focus",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.50f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::reality, 1),
        "Reality",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.00f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::sibilance, 1),
        "Sibilance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.15f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::mix, 1),
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        1.00f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::outputGain, 1),
        "Output",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }
    ));

    return layout;
}

const juce::String MorphantAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MorphantAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MorphantAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MorphantAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MorphantAudioProcessor::getTailLengthSeconds() const
{
    return 0.08;
}

int MorphantAudioProcessor::getNumPrograms()
{
    return 1;
}

int MorphantAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MorphantAudioProcessor::setCurrentProgram(int)
{
}

const juce::String MorphantAudioProcessor::getProgramName(int)
{
    return {};
}

void MorphantAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void MorphantAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleRateHz_ = sampleRate;

    dryBuffer_.setSize(2, samplesPerBlock);
    carrierBuffer_.setSize(2, samplesPerBlock);
    wetBuffer_.setSize(2, samplesPerBlock);
    modulatorMonoBuffer_.setSize(1, samplesPerBlock);
    analysisMonoBuffer_.setSize(1, samplesPerBlock);

    juce::dsp::ProcessSpec modSpec;
    modSpec.sampleRate = sampleRate;
    modSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    modSpec.numChannels = 1;

    juce::dsp::ProcessSpec carSpec = modSpec;
    carSpec.numChannels = 2;

    for (auto& band : bands_)
    {
        band.modFilter.prepare(modSpec);
        band.modFilter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        band.modFilter.setCutoffFrequency(1000.0f);
        band.modFilter.setResonance(1.0f);
        band.modFilter.reset();

        band.carFilter.prepare(carSpec);
        band.carFilter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        band.carFilter.setCutoffFrequency(1000.0f);
        band.carFilter.setResonance(1.0f);
        band.carFilter.reset();

        band.envelope = 0.0f;
    }

    activeBands_ = kStableBands;
    pitchScaleSmoother_.reset(sampleRate, 0.080);
    pitchScaleSmoother_.setCurrentAndTargetValue(1.0f);

    filterbankMakeupSmoother_.reset(sampleRate, 0.030);
    filterbankMakeupSmoother_.setCurrentAndTargetValue(1.0f);

    spectralMakeupSmoother_.reset(sampleRate, 0.020);
    spectralMakeupSmoother_.setCurrentAndTargetValue(1.0f);

    mixSmoother_.reset(sampleRate, 0.012);
    mixSmoother_.setCurrentAndTargetValue(1.0f);

    outputGainSmoother_.reset(sampleRate, 0.015);
    outputGainSmoother_.setCurrentAndTargetValue(1.0f);

    modulatorHistory_ = 0.0f;
    modulatorGateEnv_ = 0.0f;
    unvoicedEnv_ = 0.0f;
    noiseHpStateL_ = 0.0f;
    noiseHpStateR_ = 0.0f;
    noiseHpInputL_ = 0.0f;
    noiseHpInputR_ = 0.0f;
    outputLimiterEnv_ = 0.0f;
    debugLogCounter_ = 0;
    debugWasEnabled_ = false;
    debugLogFile_ = {};
    previousMode_ = ProcessingMode::Filterbank;

    auto debugDir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDocumentsDirectory)
        .getChildFile("Morphant");
    if (debugDir.createDirectory())
    {
        debugLogFile_ = debugDir.getChildFile("Morphant_debug.log");
        if (!debugLogFile_.existsAsFile())
            debugLogFile_.replaceWithText("Morphant debug log created: " + juce::Time::getCurrentTime().toString(true, true, true, true) + "\n");
    }

    ensureSpectralSetup(samplesPerBlock);
}

void MorphantAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MorphantAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
  #else
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (mainIn != mainOut)
        return false;
   #endif

    if (layouts.inputBuses.size() > 1)
    {
        const auto textureIn = layouts.getChannelSet(true, 1);
        if (!textureIn.isDisabled()
            && textureIn != juce::AudioChannelSet::mono()
            && textureIn != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
  #endif
}
#endif

int MorphantAudioProcessor::bandsForReality(float reality) noexcept
{
    if (reality < 0.20f)
        return kStableBands;
    if (reality < 0.55f)
        return kHybridBands;
    return kExperimentalBands;
}

MorphantAudioProcessor::ProcessingMode MorphantAudioProcessor::getProcessingMode() const noexcept
{
    if (const auto* modeParam = dynamic_cast<const juce::AudioParameterChoice*>(apvts_.getParameter(ParameterIDs::mode)))
        return modeParam->getIndex() == 1 ? ProcessingMode::Spectral : ProcessingMode::Filterbank;

    return ProcessingMode::Filterbank;
}

void MorphantAudioProcessor::ensureSpectralSetup(int requiredBlockSize)
{
    int minFftSize = juce::jmax(256, requiredBlockSize);
    int desiredOrder = 1;
    while ((1 << desiredOrder) < minFftSize)
        ++desiredOrder;

    desiredOrder = juce::jlimit(8, 12, desiredOrder);
    const int desiredSize = (1 << desiredOrder);

    if (spectralFft_ != nullptr && desiredSize == spectralSize_)
        return;

    spectralOrder_ = desiredOrder;
    spectralSize_ = desiredSize;
    spectralFft_ = std::make_unique<juce::dsp::FFT>(spectralOrder_);

    const size_t fftSize = static_cast<size_t>(spectralSize_);
    spectralCarrierTimeL_.assign(fftSize, juce::dsp::Complex<float>{ 0.0f, 0.0f });
    spectralCarrierTimeR_.assign(fftSize, juce::dsp::Complex<float>{ 0.0f, 0.0f });
    spectralModTime_.assign(fftSize, juce::dsp::Complex<float>{ 0.0f, 0.0f });
    spectralCarrierFreqL_.assign(fftSize, juce::dsp::Complex<float>{ 0.0f, 0.0f });
    spectralCarrierFreqR_.assign(fftSize, juce::dsp::Complex<float>{ 0.0f, 0.0f });
    spectralModFreq_.assign(fftSize, juce::dsp::Complex<float>{ 0.0f, 0.0f });
    spectralWindow_.assign(fftSize, 0.0f);
    spectralRingCarrierL_.assign(fftSize, 0.0f);
    spectralRingCarrierR_.assign(fftSize, 0.0f);
    spectralRingMod_.assign(fftSize, 0.0f);
    spectralOutAccumL_.assign(fftSize, 0.0f);
    spectralOutAccumR_.assign(fftSize, 0.0f);
    spectralModMag_.assign(fftSize, 0.0f);
    spectralModMagSmoothed_.assign(fftSize, 0.0f);
    spectralCarrierMag_.assign(fftSize, 0.0f);
    spectralCarrierMagSmoothed_.assign(fftSize, 0.0f);

    // Sine analysis/synthesis window for COLA with 50% hop.
    const float fftSizeF = static_cast<float>(spectralSize_);
    for (int i = 0; i < spectralSize_; ++i)
        spectralWindow_[static_cast<size_t>(i)] = std::sin(juce::MathConstants<float>::pi * (static_cast<float>(i) + 0.5f) / fftSizeF);

    spectralHopSize_ = juce::jmax(1, spectralSize_ / 2);
    spectralWritePos_ = 0;
    spectralSamplesSinceFrame_ = 0;
}

float MorphantAudioProcessor::computeEnvelopeCoeff(float timeMs, double sampleRate) noexcept
{
    const auto clampedMs = std::max(0.1f, timeMs);
    return 1.0f - std::exp(-1.0f / (0.001f * clampedMs * static_cast<float>(sampleRate)));
}

void MorphantAudioProcessor::configureBandFilters(int bandCount,
                                                  float pitchScale,
                                                  float focus,
                                                  float formantFollower,
                                                  float reality) noexcept
{
    const float minHz = 70.0f;
    const float nyquistCap = static_cast<float>(sampleRateHz_ * 0.49);
    const float maxTargetHz = juce::jmap(focus, 9600.0f, 16000.0f);
    const float maxHz = juce::jlimit(7000.0f, nyquistCap, maxTargetHz);

    const float denom = static_cast<float>(juce::jmax(1, bandCount - 1));
    const float ratio = std::pow(maxHz / minHz, 1.0f / denom);

    const float experimentalAmount = juce::jlimit(0.0f, 1.0f, (reality - 0.55f) / 0.45f);
    const float overlapScale = juce::jmap(focus, 4.0f, 2.6f);

    for (int i = 0; i < bandCount; ++i)
    {
        auto& band = bands_[static_cast<size_t>(i)];
        const float position = static_cast<float>(i) / denom;

        const float formantWarp = 1.0f + formantFollower * 0.08f
                                        * std::sin(juce::MathConstants<float>::twoPi * position);

        float centerHz = minHz * std::pow(ratio, static_cast<float>(i)) * pitchScale * formantWarp;
        centerHz = juce::jlimit(minHz, maxHz, centerHz);

        const float spacing = juce::jmax(0.005f, ratio - 1.0f);
        const float bandwidthHz = centerHz * spacing * overlapScale + 120.0f;
        float resonance = centerHz / juce::jmax(80.0f, bandwidthHz);
        resonance *= 0.90f + 0.20f * (1.0f - std::abs(2.0f * position - 1.0f));

        if (experimentalAmount > 0.0f)
            resonance *= 1.0f + experimentalAmount * 0.15f * std::sin(19.0f * position);

        resonance = juce::jlimit(0.75f, 6.0f, resonance);

        band.modFilter.setCutoffFrequency(centerHz);
        band.modFilter.setResonance(resonance);

        band.carFilter.setCutoffFrequency(centerHz);
        band.carFilter.setResonance(resonance);
    }
}

float MorphantAudioProcessor::estimatePitchHz(const float* mono, int numSamples, float* confidenceOut) const noexcept
{
    if (mono == nullptr || numSamples < 128)
    {
        if (confidenceOut != nullptr)
            *confidenceOut = 0.0f;
        return 0.0f;
    }

    constexpr float minPitchHz = 70.0f;
    constexpr float maxPitchHz = 420.0f;
    constexpr int decimation = 2;

    const int minLag = juce::jmax(2, static_cast<int>(sampleRateHz_ / maxPitchHz));
    const int maxLag = juce::jmin(numSamples - 2, static_cast<int>(sampleRateHz_ / minPitchHz));

    if (minLag >= maxLag)
    {
        if (confidenceOut != nullptr)
            *confidenceOut = 0.0f;
        return 0.0f;
    }

    double mean = 0.0;
    int count = 0;
    for (int i = 0; i < numSamples; i += decimation)
    {
        mean += mono[i];
        ++count;
    }

    if (count == 0)
    {
        if (confidenceOut != nullptr)
            *confidenceOut = 0.0f;
        return 0.0f;
    }

    mean /= static_cast<double>(count);

    float bestCorrelation = 0.0f;
    int bestLag = 0;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double ac = 0.0;
        double e1 = 0.0;
        double e2 = 0.0;

        for (int i = 0; i + lag < numSamples; i += decimation)
        {
            const float a = mono[i] - static_cast<float>(mean);
            const float b = mono[i + lag] - static_cast<float>(mean);

            ac += static_cast<double>(a * b);
            e1 += static_cast<double>(a * a);
            e2 += static_cast<double>(b * b);
        }

        const double denom = std::sqrt(e1 * e2) + 1.0e-12;
        const float corr = static_cast<float>(ac / denom);

        if (corr > bestCorrelation)
        {
            bestCorrelation = corr;
            bestLag = lag;
        }
    }

    if (confidenceOut != nullptr)
        *confidenceOut = juce::jlimit(0.0f, 1.0f, bestCorrelation);

    if (bestCorrelation < 0.22f || bestLag <= 0)
        return 0.0f;

    const float pitchHz = static_cast<float>(sampleRateHz_ / static_cast<double>(bestLag));
    return std::isfinite(pitchHz) ? pitchHz : 0.0f;
}

float MorphantAudioProcessor::estimateBrightness(const float* mono, int numSamples) noexcept
{
    if (mono == nullptr || numSamples < 2)
        return 0.5f;

    double absSum = 0.0;
    double diffSum = 0.0;

    float prev = mono[0];
    for (int i = 0; i < numSamples; ++i)
    {
        const float v = mono[i];
        absSum += std::abs(v);
        diffSum += std::abs(v - prev);
        prev = v;
    }

    const float ratio = static_cast<float>(diffSum / (absSum + 1.0e-9));
    return juce::jlimit(0.0f, 1.0f, ratio * 0.75f);
}

void MorphantAudioProcessor::resetEnvelopes() noexcept
{
    for (auto& band : bands_)
        band.envelope = 0.0f;
}

void MorphantAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    if (numSamples > spectralSize_)
        ensureSpectralSetup(numSamples);

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);

    auto mainInput = getBusBuffer(buffer, true, 0);
    auto mainOutput = getBusBuffer(buffer, false, 0);

    const int inChannels = mainInput.getNumChannels();
    const int outChannels = mainOutput.getNumChannels();

    if (inChannels == 0 || outChannels == 0)
    {
        mainOutput.clear();
        return;
    }

    if (numSamples > carrierBuffer_.getNumSamples())
    {
        carrierBuffer_.setSize(2, numSamples, false, false, true);
        dryBuffer_.setSize(2, numSamples, false, false, true);
        wetBuffer_.setSize(2, numSamples, false, false, true);
        modulatorMonoBuffer_.setSize(1, numSamples, false, false, true);
        analysisMonoBuffer_.setSize(1, numSamples, false, false, true);
    }

    const float* mainL = mainInput.getReadPointer(0);
    const float* mainR = mainInput.getReadPointer(inChannels > 1 ? 1 : 0);

    const float* sideL = nullptr;
    const float* sideR = nullptr;
    bool sidechainAvailable = false;

    if (getBusCount(true) > 1)
    {
        if (const auto* textureBus = getBus(true, 1); textureBus != nullptr && textureBus->isEnabled())
        {
            auto texture = getBusBuffer(buffer, true, 1);
            if (texture.getNumChannels() > 0)
            {
                sidechainAvailable = true;
                const int textureCh = texture.getNumChannels();
                sideL = texture.getReadPointer(0);
                sideR = texture.getReadPointer(textureCh > 1 ? 1 : 0);
            }
        }
    }

    // Deterministic routing to match classic DAW vocoder workflows:
    // main input (track insert) = modulator voice, sidechain = carrier synth.
    // If sidechain is not present, Morphant falls back to self-vocoding on main.
    const float* modulatorSrcL = mainL;
    const float* modulatorSrcR = mainR;
    const float* carrierSrcL = sidechainAvailable ? sideL : mainL;
    const float* carrierSrcR = sidechainAvailable ? sideR : mainR;

    float* carrierL = carrierBuffer_.getWritePointer(0);
    float* carrierR = carrierBuffer_.getWritePointer(1);
    float* dryL = dryBuffer_.getWritePointer(0);
    float* dryR = dryBuffer_.getWritePointer(1);

    juce::FloatVectorOperations::copy(carrierL, carrierSrcL, numSamples);
    juce::FloatVectorOperations::copy(carrierR, carrierSrcR, numSamples);
    juce::FloatVectorOperations::copy(dryL, carrierSrcL, numSamples);
    juce::FloatVectorOperations::copy(dryR, carrierSrcR, numSamples);

    double carrierEnergy = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        const float mono = 0.5f * (carrierL[i] + carrierR[i]);
        carrierEnergy += static_cast<double>(mono * mono);
    }
    const float carrierRms = std::sqrt(static_cast<float>(carrierEnergy / static_cast<double>(juce::jmax(1, numSamples))) + 1.0e-12f);

    float* modMono = modulatorMonoBuffer_.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i)
        modMono[i] = 0.5f * (modulatorSrcL[i] + modulatorSrcR[i]);

    const float merge = apvts_.getRawParameterValue(ParameterIDs::merge)->load();
    const float glue = apvts_.getRawParameterValue(ParameterIDs::glue)->load();
    const float pitchFollower = apvts_.getRawParameterValue(ParameterIDs::pitchFollower)->load();
    const float formantFollower = apvts_.getRawParameterValue(ParameterIDs::formantFollower)->load();
    const float focus = apvts_.getRawParameterValue(ParameterIDs::focus)->load();
    const float reality = apvts_.getRawParameterValue(ParameterIDs::reality)->load();
    const float sibilance = apvts_.getRawParameterValue(ParameterIDs::sibilance)->load();
    const float mix = apvts_.getRawParameterValue(ParameterIDs::mix)->load();
    const float outputGainDb = apvts_.getRawParameterValue(ParameterIDs::outputGain)->load();
    const bool debugMode = apvts_.getRawParameterValue(ParameterIDs::debugMode)->load() > 0.5f;
    const ProcessingMode mode = getProcessingMode();

    if (mode != previousMode_)
    {
        resetEnvelopes();
        modulatorGateEnv_ = 0.0f;
        unvoicedEnv_ = 0.0f;
        std::fill(spectralRingCarrierL_.begin(), spectralRingCarrierL_.end(), 0.0f);
        std::fill(spectralRingCarrierR_.begin(), spectralRingCarrierR_.end(), 0.0f);
        std::fill(spectralRingMod_.begin(), spectralRingMod_.end(), 0.0f);
        std::fill(spectralOutAccumL_.begin(), spectralOutAccumL_.end(), 0.0f);
        std::fill(spectralOutAccumR_.begin(), spectralOutAccumR_.end(), 0.0f);
        spectralWritePos_ = 0;
        spectralSamplesSinceFrame_ = 0;
        outputLimiterEnv_ = 0.0f;
        filterbankMakeupSmoother_.setCurrentAndTargetValue(1.0f);
        spectralMakeupSmoother_.setCurrentAndTargetValue(1.0f);
        mixSmoother_.setCurrentAndTargetValue(mix);
        previousMode_ = mode;
    }

    // Traditional channel vocoders typically pre-emphasize the modulator so consonants
    // and high-formant detail drive envelopes more effectively.
    float* modAnalysis = analysisMonoBuffer_.getWritePointer(0);
    const float preEmphasis = juce::jmap(focus, 0.86f, 0.97f);
    float modHistory = modulatorHistory_;

    for (int i = 0; i < numSamples; ++i)
    {
        const float raw = modMono[i];
        const float emphasized = raw - preEmphasis * modHistory;
        modHistory = raw;
        modAnalysis[i] = emphasized;
    }

    modulatorHistory_ = modHistory;

    double modEnergy = 0.0;
    for (int i = 0; i < numSamples; ++i)
        modEnergy += static_cast<double>(modMono[i] * modMono[i]);

    const float modRms = std::sqrt(static_cast<float>(modEnergy / static_cast<double>(juce::jmax(1, numSamples))) + 1.0e-12f);
    const bool modulatorActive = modRms > 0.0030f;

    const int desiredBands = bandsForReality(reality);
    if (desiredBands != activeBands_)
    {
        activeBands_ = desiredBands;
        resetEnvelopes();
    }

    float pitchConfidence = 0.0f;
    const float detectedPitchHz = modulatorActive
        ? estimatePitchHz(modMono, numSamples, &pitchConfidence)
        : 0.0f;

    float targetPitchScale = pitchScaleSmoother_.getTargetValue();
    if (detectedPitchHz > 0.0f && pitchConfidence > 0.28f && modulatorActive)
    {
        const float pitchRatio = juce::jlimit(0.5f, 2.0f, detectedPitchHz / 120.0f);
        const float desiredScale = std::pow(pitchRatio, pitchFollower * 0.75f);
        const float blend = juce::jlimit(0.0f, 1.0f, (pitchConfidence - 0.28f) / 0.55f);
        targetPitchScale = juce::jmap(blend, targetPitchScale, desiredScale);
    }
    else
    {
        targetPitchScale = juce::jmap(0.06f, targetPitchScale, 1.0f);
    }

    pitchScaleSmoother_.setTargetValue(juce::jlimit(0.5f, 2.0f, targetPitchScale));
    const float pitchScale = pitchScaleSmoother_.getNextValue();

    wetBuffer_.clear();
    float* wetL = wetBuffer_.getWritePointer(0);
    float* wetR = wetBuffer_.getWritePointer(1);

    const float brightness = estimateBrightness(modMono, numSamples);
    const float formantTilt = (brightness - 0.5f) * 2.0f * formantFollower;
    const float experimentalAmount = juce::jlimit(0.0f, 1.0f, (reality - 0.55f) / 0.45f);
    int spectralFramesProcessed = 0;
    float debugVoiceGate = 0.0f;
    float debugBandEnvMean = 0.0f;
    float debugBandEnvPeak = 0.0f;
    float debugFilterbankMakeup = 1.0f;

    if (mode == ProcessingMode::Filterbank)
    {
        configureBandFilters(activeBands_, pitchScale, focus, formantFollower, reality);

        const float attackMs = juce::jmap(glue, 0.30f, 9.0f);
        const float releaseMs = juce::jmap(glue, 18.0f, 170.0f);
        const float attackCoeff = computeEnvelopeCoeff(attackMs, sampleRateHz_);
        const float releaseCoeff = computeEnvelopeCoeff(releaseMs, sampleRateHz_);
        const float gateAttackCoeff = computeEnvelopeCoeff(2.0f, sampleRateHz_);
        const float gateReleaseCoeff = computeEnvelopeCoeff(35.0f + 90.0f * glue, sampleRateHz_);
        const float detectorBlend = 0.12f + 0.28f * focus;
        const float denom = static_cast<float>(juce::jmax(1, activeBands_ - 1));
        const float blockVoiceGate = juce::jlimit(0.0f, 1.0f, (modRms - 0.006f) * 18.0f);

        std::array<float, kMaxBands> envelopeSnapshot{};
        std::array<float, kMaxBands> staticBandGain{};
        const float envelopeScale = juce::jmap(merge, 2.8f, 8.5f)
                                  * std::sqrt(static_cast<float>(activeBands_) / static_cast<float>(kStableBands));
        const float envelopeShape = juce::jmap(focus, 0.86f, 1.12f);
        const float bandNormalization = 0.42f / std::sqrt(static_cast<float>(activeBands_));
        const float morphFloor = juce::jmap(focus, 0.015f, 0.045f);

        float gateEnv = modulatorGateEnv_;
        double gateSum = 0.0;
        double sampleEnvMeanSum = 0.0;
        float peakBandEnv = 0.0f;

        for (int b = 0; b < activeBands_; ++b)
        {
            const float position = static_cast<float>(b) / denom;
            const float tiltDb = formantTilt * (position - 0.5f) * 8.0f;
            const float tiltGain = juce::Decibels::decibelsToGain(tiltDb);
            const float focusBoost = 1.0f + focus * (1.0f - std::abs(2.0f * position - 1.0f)) * 0.16f;
            staticBandGain[static_cast<size_t>(b)] = bandNormalization * tiltGain * focusBoost;
        }

        for (int s = 0; s < numSamples; ++s)
        {
            const float detectorInput = modMono[s] + detectorBlend * modAnalysis[s];
            const float gateDetector = std::abs(detectorInput);
            const float gateCoeff = (gateDetector > gateEnv) ? gateAttackCoeff : gateReleaseCoeff;
            gateEnv += gateCoeff * (gateDetector - gateEnv);

            const float voiceGate = modulatorActive
                ? juce::jlimit(0.0f, 1.0f, (gateEnv - 0.0040f) * 24.0f) * blockVoiceGate
                : 0.0f;

            float sampleEnvMean = 0.0f;

            for (int b = 0; b < activeBands_; ++b)
            {
                auto& band = bands_[static_cast<size_t>(b)];
                float env = band.envelope;

                const float detector = std::abs(band.modFilter.processSample(0, detectorInput));
                const float coeff = (detector > env) ? attackCoeff : releaseCoeff;
                env += coeff * (detector - env);
                band.envelope = env;

                envelopeSnapshot[static_cast<size_t>(b)] = env;
                sampleEnvMean += env;
                peakBandEnv = juce::jmax(peakBandEnv, env);
            }

            sampleEnvMean /= static_cast<float>(juce::jmax(1, activeBands_));

            float wetSampleL = 0.0f;
            float wetSampleR = 0.0f;
            for (int b = 0; b < activeBands_; ++b)
            {
                auto& band = bands_[static_cast<size_t>(b)];

                float normalizedEnvelope = juce::jlimit(0.0f, 2.0f, envelopeSnapshot[static_cast<size_t>(b)] * envelopeScale);
                float shapedEnvelope = std::pow(normalizedEnvelope, envelopeShape);

                if (experimentalAmount > 0.0f)
                    shapedEnvelope = std::pow(shapedEnvelope, 1.0f - 0.12f * experimentalAmount);

                const float morphGain = juce::jmap(merge, morphFloor, shapedEnvelope) * voiceGate;
                const float bandGain = staticBandGain[static_cast<size_t>(b)] * morphGain;

                wetSampleL += band.carFilter.processSample(0, carrierL[s]) * bandGain;
                wetSampleR += band.carFilter.processSample(1, carrierR[s]) * bandGain;
            }

            wetL[s] = wetSampleL;
            wetR[s] = wetSampleR;
            gateSum += static_cast<double>(voiceGate);
            sampleEnvMeanSum += static_cast<double>(sampleEnvMean);
        }

        modulatorGateEnv_ = gateEnv;
        debugVoiceGate = static_cast<float>(gateSum / static_cast<double>(juce::jmax(1, numSamples)));
        debugBandEnvMean = static_cast<float>(sampleEnvMeanSum / static_cast<double>(juce::jmax(1, numSamples)));
        debugBandEnvPeak = peakBandEnv;

        double filterbankWetEnergy = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            const float mono = 0.5f * (wetL[i] + wetR[i]);
            filterbankWetEnergy += static_cast<double>(mono * mono);
        }

        const float filterbankWetRms = std::sqrt(static_cast<float>(filterbankWetEnergy / static_cast<double>(juce::jmax(1, numSamples))) + 1.0e-12f);
        const float desiredFilterbankRms = modulatorActive
            ? juce::jmax(0.02f, carrierRms * (0.26f + 0.40f * merge))
            : 0.0f;
        float filterbankMakeup = modulatorActive
            ? (desiredFilterbankRms / juce::jmax(1.0e-5f, filterbankWetRms))
            : 0.0f;
        filterbankMakeup = juce::jlimit(0.0f, 6.0f, filterbankMakeup);
        filterbankMakeupSmoother_.setTargetValue(filterbankMakeup);
        debugFilterbankMakeup = filterbankMakeup;

        for (int i = 0; i < numSamples; ++i)
        {
            const float g = filterbankMakeupSmoother_.getNextValue();
            wetL[i] *= g;
            wetR[i] *= g;
        }
    }
    else
    {
        ensureSpectralSetup(numSamples);

        const int fftSize = spectralSize_;
        const int halfBins = fftSize / 2;
        const float detectorBlend = 0.20f + 0.30f * focus;
        const float formantTiltScale = formantFollower * 7.0f;
        const float transferExponent = juce::jmap(focus, 0.78f, 1.12f);
        const float spectralHighFloor = juce::jlimit(0.0f, 0.40f, 0.06f + 0.22f * sibilance);

        auto processSpectralFrame = [&]()
        {
            const int frameStart = (spectralWritePos_ + 1) % fftSize;

            for (int i = 0; i < fftSize; ++i)
            {
                const int ringPos = (frameStart + i) % fftSize;
                const float window = spectralWindow_[static_cast<size_t>(i)];

                spectralCarrierTimeL_[static_cast<size_t>(i)] =
                    juce::dsp::Complex<float>{ spectralRingCarrierL_[static_cast<size_t>(ringPos)] * window, 0.0f };
                spectralCarrierTimeR_[static_cast<size_t>(i)] =
                    juce::dsp::Complex<float>{ spectralRingCarrierR_[static_cast<size_t>(ringPos)] * window, 0.0f };
                spectralModTime_[static_cast<size_t>(i)] =
                    juce::dsp::Complex<float>{ spectralRingMod_[static_cast<size_t>(ringPos)] * window, 0.0f };
            }

            spectralFft_->perform(spectralCarrierTimeL_.data(), spectralCarrierFreqL_.data(), false);
            spectralFft_->perform(spectralCarrierTimeR_.data(), spectralCarrierFreqR_.data(), false);
            spectralFft_->perform(spectralModTime_.data(), spectralModFreq_.data(), false);

            for (int k = 0; k <= halfBins; ++k)
            {
                const float modMag = std::abs(spectralModFreq_[static_cast<size_t>(k)]);
                spectralModMag_[static_cast<size_t>(k)] = modMag;

                const float carrierMag = 0.5f * (std::abs(spectralCarrierFreqL_[static_cast<size_t>(k)])
                                               + std::abs(spectralCarrierFreqR_[static_cast<size_t>(k)]));
                spectralCarrierMag_[static_cast<size_t>(k)] = carrierMag;
            }

            const int baseSmoothingRadius = juce::jlimit(1, 10, juce::roundToInt(juce::jmap(1.0f - focus, 3.0f, 9.0f)));
            for (int k = 0; k <= halfBins; ++k)
            {
                const float position = static_cast<float>(k) / static_cast<float>(juce::jmax(1, halfBins));
                const float highDetailFactor = juce::jmap(position, 1.0f, 0.35f);
                const int smoothingRadius = juce::jmax(1, juce::roundToInt(static_cast<float>(baseSmoothingRadius) * highDetailFactor));

                float smoothedMod = 0.0f;
                float smoothedCarrier = 0.0f;
                float weightSum = 0.0f;
                for (int r = -smoothingRadius; r <= smoothingRadius; ++r)
                {
                    const int idx = juce::jlimit(0, halfBins, k + r);
                    const float weight = 1.0f - (std::abs(static_cast<float>(r)) / static_cast<float>(smoothingRadius + 1));
                    smoothedMod += spectralModMag_[static_cast<size_t>(idx)] * weight;
                    smoothedCarrier += spectralCarrierMag_[static_cast<size_t>(idx)] * weight;
                    weightSum += weight;
                }
                const float norm = juce::jmax(1.0e-6f, weightSum);
                spectralModMagSmoothed_[static_cast<size_t>(k)] = smoothedMod / norm;
                spectralCarrierMagSmoothed_[static_cast<size_t>(k)] = smoothedCarrier / norm;
            }

            const float inactiveDamp = 1.0f - merge * 0.92f;
            const float maxTransfer = juce::jmap(reality, 2.5f, 5.5f);
            const float maxOutScale = 1.0f + 3.0f * merge;

            auto shapeSpectrum = [&](std::vector<juce::dsp::Complex<float>>& carrierSpectrum)
            {
                for (int k = 0; k <= halfBins; ++k)
                {
                    const auto carrierBin = carrierSpectrum[static_cast<size_t>(k)];
                    const float carrierMag = std::abs(carrierBin) + 1.0e-9f;
                    const float carrierPhase = std::arg(carrierBin);

                    float outMag = carrierMag;
                    if (modulatorActive)
                    {
                        const float sourceBin = juce::jlimit(0.0f, static_cast<float>(halfBins),
                                                             static_cast<float>(k) / juce::jmax(0.55f, pitchScale));
                        const int lower = juce::jlimit(0, halfBins, static_cast<int>(sourceBin));
                        const int upper = juce::jmin(halfBins, lower + 1);
                        const float frac = sourceBin - static_cast<float>(lower);

                        const float mMag = juce::jmap(frac,
                            spectralModMagSmoothed_[static_cast<size_t>(lower)],
                            spectralModMagSmoothed_[static_cast<size_t>(upper)]);

                        const float carrierRefMag = spectralCarrierMagSmoothed_[static_cast<size_t>(juce::jlimit(0, halfBins, k))] + 1.0e-6f;
                        float transfer = mMag / carrierRefMag;
                        transfer = std::pow(juce::jlimit(0.0f, maxTransfer, transfer), transferExponent);
                        transfer = juce::jlimit(0.0f, maxTransfer, transfer);

                        float targetMag = carrierMag * juce::jmap(merge, 1.0f, transfer);

                        const float position = static_cast<float>(k) / static_cast<float>(juce::jmax(1, halfBins));
                        const float tiltDb = (position - 0.5f) * formantTiltScale;
                        targetMag *= juce::Decibels::decibelsToGain(tiltDb);
                        const float hfFloor = carrierMag * juce::jmap(position, 0.0f, spectralHighFloor);
                        targetMag = juce::jmax(targetMag, hfFloor);
                        outMag = juce::jlimit(0.0f, carrierMag * maxOutScale, targetMag);
                    }
                    else
                    {
                        outMag = carrierMag * inactiveDamp;
                    }

                    if (k == 0 || k == halfBins)
                    {
                        carrierSpectrum[static_cast<size_t>(k)] = juce::dsp::Complex<float>{ outMag, 0.0f };
                    }
                    else
                    {
                        const auto outBin = std::polar(outMag, carrierPhase);
                        carrierSpectrum[static_cast<size_t>(k)] = outBin;
                        carrierSpectrum[static_cast<size_t>(fftSize - k)] = std::conj(outBin);
                    }
                }
            };

            shapeSpectrum(spectralCarrierFreqL_);
            shapeSpectrum(spectralCarrierFreqR_);

            spectralFft_->perform(spectralCarrierFreqL_.data(), spectralCarrierTimeL_.data(), true);
            spectralFft_->perform(spectralCarrierFreqR_.data(), spectralCarrierTimeR_.data(), true);

            for (int i = 0; i < fftSize; ++i)
            {
                const int ringPos = (frameStart + i) % fftSize;
                const float window = spectralWindow_[static_cast<size_t>(i)];
                spectralOutAccumL_[static_cast<size_t>(ringPos)] +=
                    spectralCarrierTimeL_[static_cast<size_t>(i)].real() * window;
                spectralOutAccumR_[static_cast<size_t>(ringPos)] +=
                    spectralCarrierTimeR_[static_cast<size_t>(i)].real() * window;
            }
        };

        for (int s = 0; s < numSamples; ++s)
        {
            spectralRingCarrierL_[static_cast<size_t>(spectralWritePos_)] = carrierL[s];
            spectralRingCarrierR_[static_cast<size_t>(spectralWritePos_)] = carrierR[s];
            spectralRingMod_[static_cast<size_t>(spectralWritePos_)] = modMono[s] + detectorBlend * modAnalysis[s];

            ++spectralSamplesSinceFrame_;
            if (spectralSamplesSinceFrame_ >= spectralHopSize_)
            {
                spectralSamplesSinceFrame_ = 0;
                processSpectralFrame();
                ++spectralFramesProcessed;
            }

            wetL[s] = spectralOutAccumL_[static_cast<size_t>(spectralWritePos_)];
            wetR[s] = spectralOutAccumR_[static_cast<size_t>(spectralWritePos_)];
            spectralOutAccumL_[static_cast<size_t>(spectralWritePos_)] = 0.0f;
            spectralOutAccumR_[static_cast<size_t>(spectralWritePos_)] = 0.0f;

            spectralWritePos_ = (spectralWritePos_ + 1) % fftSize;
        }

        double spectralWetEnergy = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            const float mono = 0.5f * (wetL[i] + wetR[i]);
            spectralWetEnergy += static_cast<double>(mono * mono);
        }

        const float spectralWetRms = std::sqrt(static_cast<float>(spectralWetEnergy / static_cast<double>(juce::jmax(1, numSamples))) + 1.0e-12f);
        const float desiredSpectralRms = modulatorActive
            ? juce::jmax(0.04f, carrierRms * (0.55f + 0.65f * merge))
            : 0.0f;
        float spectralMakeup = modulatorActive
            ? (desiredSpectralRms / juce::jmax(1.0e-5f, spectralWetRms))
            : 0.0f;
        spectralMakeup = juce::jlimit(0.0f, 16.0f, spectralMakeup);
        spectralMakeupSmoother_.setTargetValue(spectralMakeup);

        for (int i = 0; i < numSamples; ++i)
        {
            const float g = spectralMakeupSmoother_.getNextValue();
            wetL[i] *= g;
            wetR[i] *= g;
        }
    }

    double wetEnergyPreOutput = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        const float mono = 0.5f * (wetL[i] + wetR[i]);
        wetEnergyPreOutput += static_cast<double>(mono * mono);
    }
    const float wetRmsPreOutput = std::sqrt(static_cast<float>(wetEnergyPreOutput / static_cast<double>(juce::jmax(1, numSamples))) + 1.0e-12f);

    outputGainSmoother_.setTargetValue(juce::Decibels::decibelsToGain(outputGainDb));
    mixSmoother_.setTargetValue(mix);

    const bool stereoOutput = outChannels > 1;
    float* outL = mainOutput.getWritePointer(0);
    float* outR = stereoOutput ? mainOutput.getWritePointer(1) : nullptr;

    const float wetDrive = 1.0f + experimentalAmount * 1.8f;
    const float wetWarpBlend = experimentalAmount * 0.4f;
    const float unvoicedAttackCoeff = computeEnvelopeCoeff(0.7f + glue * 1.8f, sampleRateHz_);
    const float unvoicedReleaseCoeff = computeEnvelopeCoeff(18.0f + glue * 110.0f, sampleRateHz_);
    const float noiseHighpassCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi * 4000.0f / static_cast<float>(sampleRateHz_));
    const float sibilanceBaseGain = (0.0010f + 0.028f * sibilance)
                                  * (0.70f + 0.30f * focus)
                                  * (1.0f + 0.30f * experimentalAmount);
    const float limiterAttackCoeff = computeEnvelopeCoeff(0.12f, sampleRateHz_);
    const float limiterReleaseCoeff = computeEnvelopeCoeff(90.0f, sampleRateHz_);
    constexpr float limiterThreshold = 0.92f;

    float unvoicedEnv = unvoicedEnv_;
    float limiterEnv = outputLimiterEnv_;
    float minLimiterGain = 1.0f;
    double outEnergy = 0.0;

    for (int s = 0; s < numSamples; ++s)
    {
        float wetSampleL = wetL[s];
        float wetSampleR = wetR[s];

        if (wetWarpBlend > 0.0f)
        {
            const float warpedL = std::tanh(wetSampleL * wetDrive);
            const float warpedR = std::tanh(wetSampleR * wetDrive);
            wetSampleL = juce::jmap(wetWarpBlend, wetSampleL, warpedL);
            wetSampleR = juce::jmap(wetWarpBlend, wetSampleR, warpedR);
        }

        // Approximate voiced/unvoiced split: emphasize noisy/fricative regions with
        // high-passed noise, like traditional vocoder consonant enhancement paths.
        const float unvoicedDetector = modulatorActive
            ? juce::jmax(0.0f, std::abs(modAnalysis[s]) - 0.48f * std::abs(modMono[s]))
            : 0.0f;
        const float uvCoeff = (unvoicedDetector > unvoicedEnv) ? unvoicedAttackCoeff : unvoicedReleaseCoeff;
        unvoicedEnv += uvCoeff * (unvoicedDetector - unvoicedEnv);
        const float unvoicedNorm = juce::jlimit(0.0f, 1.0f, unvoicedEnv * (6.5f + 2.0f * focus));
        const float sibilanceGain = sibilanceBaseGain * unvoicedNorm;

        const float noiseInputL = noiseRandom_.nextFloat() * 2.0f - 1.0f;
        const float noiseInputR = noiseRandom_.nextFloat() * 2.0f - 1.0f;

        noiseHpStateL_ = noiseHighpassCoeff * (noiseHpStateL_ + noiseInputL - noiseHpInputL_);
        noiseHpStateR_ = noiseHighpassCoeff * (noiseHpStateR_ + noiseInputR - noiseHpInputR_);
        noiseHpInputL_ = noiseInputL;
        noiseHpInputR_ = noiseInputR;

        wetSampleL += noiseHpStateL_ * sibilanceGain;
        wetSampleR += noiseHpStateR_ * sibilanceGain;

        const float mixValue = mixSmoother_.getNextValue();
        const float gain = outputGainSmoother_.getNextValue();
        float outSampleL = ((1.0f - mixValue) * dryL[s] + mixValue * wetSampleL) * gain;
        float outSampleR = stereoOutput
            ? ((1.0f - mixValue) * dryR[s] + mixValue * wetSampleR) * gain
            : outSampleL;

        const float absPeak = juce::jmax(std::abs(outSampleL), std::abs(outSampleR));
        const float limiterCoeff = (absPeak > limiterEnv) ? limiterAttackCoeff : limiterReleaseCoeff;
        limiterEnv += limiterCoeff * (absPeak - limiterEnv);

        if (limiterEnv > limiterThreshold)
        {
            const float limiterGain = limiterThreshold / (limiterEnv + 1.0e-9f);
            outSampleL *= limiterGain;
            outSampleR *= limiterGain;
            minLimiterGain = juce::jmin(minLimiterGain, limiterGain);
        }

        outL[s] = outSampleL;
        if (stereoOutput)
            outR[s] = outSampleR;

        const float monoOut = 0.5f * (outSampleL + outSampleR);
        outEnergy += static_cast<double>(monoOut * monoOut);
    }

    unvoicedEnv_ = unvoicedEnv;
    outputLimiterEnv_ = limiterEnv;

    juce::File debugFile = debugLogFile_;
    if (debugFile.getFullPathName().isEmpty())
    {
        debugFile = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory)
            .getChildFile("Morphant_debug.log");
        debugLogFile_ = debugFile;
    }
    debugFile.getParentDirectory().createDirectory();

    if (debugMode)
    {
        if (!debugWasEnabled_)
        {
            debugWasEnabled_ = true;
            debugLogCounter_ = 0;
            debugFile.replaceWithText(
                "Morphant debug started: " + juce::Time::getCurrentTime().toString(true, true, true, true)
                + "\nPath: " + debugFile.getFullPathName()
                + "\n");
        }

        ++debugLogCounter_;
        if (debugLogCounter_ >= 8)
        {
            debugLogCounter_ = 0;

            const float outRms = std::sqrt(static_cast<float>(outEnergy / static_cast<double>(juce::jmax(1, numSamples))) + 1.0e-12f);
            const float limiterReductionDb = juce::Decibels::gainToDecibels(juce::jmax(minLimiterGain, 1.0e-6f));
            const juce::String modeName = (mode == ProcessingMode::Filterbank) ? "Filterbank" : "Spectral";
            const int debugFrames = (mode == ProcessingMode::Spectral) ? spectralFramesProcessed : -1;
            const float debugFilterbankMakeupOut = (mode == ProcessingMode::Filterbank) ? debugFilterbankMakeup : 0.0f;
            const float debugSpectralMakeup = (mode == ProcessingMode::Spectral) ? spectralMakeupSmoother_.getCurrentValue() : 0.0f;
            const juce::String line =
                "MorphantDBG t=" + juce::Time::getCurrentTime().toString(true, true, true, true)
                + " mode=" + modeName
                + " sidechain=" + juce::String(sidechainAvailable ? "1" : "0")
                + " bands=" + juce::String(activeBands_)
                + " frames=" + juce::String(debugFrames)
                + " carRMS=" + juce::String(carrierRms, 5)
                + " modRMS=" + juce::String(modRms, 5)
                + " wetRMS=" + juce::String(wetRmsPreOutput, 5)
                + " outRMS=" + juce::String(outRms, 5)
                + " limiterGRdB=" + juce::String(limiterReductionDb, 2)
                + " gate=" + juce::String(debugVoiceGate, 3)
                + " envMean=" + juce::String(debugBandEnvMean, 5)
                + " envPeak=" + juce::String(debugBandEnvPeak, 5)
                + " fbMk=" + juce::String(debugFilterbankMakeupOut, 3)
                + " spMk=" + juce::String(debugSpectralMakeup, 3);

            DBG(line);
            juce::Logger::writeToLog(line);
            debugFile.appendText(line + "\n");
        }
    }
    else
    {
        debugLogCounter_ = 0;
        debugWasEnabled_ = false;
    }
}

bool MorphantAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MorphantAudioProcessor::createEditor()
{
    return new MorphantAudioProcessorEditor(*this);
}

void MorphantAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MorphantAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(apvts_.state.getType()))
        apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MorphantAudioProcessor();
}
