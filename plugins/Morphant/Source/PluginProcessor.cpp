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

    mixSmoother_.reset(sampleRate, 0.012);
    mixSmoother_.setCurrentAndTargetValue(1.0f);

    outputGainSmoother_.reset(sampleRate, 0.015);
    outputGainSmoother_.setCurrentAndTargetValue(1.0f);

    modulatorHistory_ = 0.0f;
    unvoicedEnv_ = 0.0f;
    noiseHpStateL_ = 0.0f;
    noiseHpStateR_ = 0.0f;
    noiseHpInputL_ = 0.0f;
    noiseHpInputR_ = 0.0f;
    outputLimiterEnv_ = 0.0f;
    previousMode_ = ProcessingMode::Filterbank;

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
    const float maxHz = juce::jlimit(3000.0f, static_cast<float>(sampleRateHz_ * 0.45), 14000.0f);

    const float denom = static_cast<float>(juce::jmax(1, bandCount - 1));
    const float ratio = std::pow(maxHz / minHz, 1.0f / denom);

    const float experimentalAmount = juce::jlimit(0.0f, 1.0f, (reality - 0.55f) / 0.45f);
    const float qLow = juce::jmap(focus, 1.4f, 3.4f);
    const float qHigh = juce::jmap(focus, 2.8f, 8.8f);

    for (int i = 0; i < bandCount; ++i)
    {
        auto& band = bands_[static_cast<size_t>(i)];
        const float position = static_cast<float>(i) / denom;

        const float formantWarp = 1.0f + formantFollower * 0.08f
                                        * std::sin(juce::MathConstants<float>::twoPi * position);

        float centerHz = minHz * std::pow(ratio, static_cast<float>(i)) * pitchScale * formantWarp;
        centerHz = juce::jlimit(minHz, maxHz, centerHz);

        float resonance = juce::jmap(position, qLow, qHigh);
        resonance *= 0.90f + 0.25f * (1.0f - std::abs(2.0f * position - 1.0f));

        if (experimentalAmount > 0.0f)
            resonance *= 1.0f + experimentalAmount * 0.2f * std::sin(19.0f * position);

        resonance = juce::jlimit(0.6f, 12.0f, resonance);

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

    // Deterministic routing to match conventional vocoders:
    // main input = carrier, sidechain input = modulator.
    // If sidechain is not present, Morphant falls back to self-vocoding on main.
    const float* carrierSrcL = mainL;
    const float* carrierSrcR = mainR;
    const float* modulatorSrcL = sidechainAvailable ? sideL : mainL;
    const float* modulatorSrcR = sidechainAvailable ? sideR : mainR;

    float* carrierL = carrierBuffer_.getWritePointer(0);
    float* carrierR = carrierBuffer_.getWritePointer(1);
    float* dryL = dryBuffer_.getWritePointer(0);
    float* dryR = dryBuffer_.getWritePointer(1);

    juce::FloatVectorOperations::copy(carrierL, carrierSrcL, numSamples);
    juce::FloatVectorOperations::copy(carrierR, carrierSrcR, numSamples);
    juce::FloatVectorOperations::copy(dryL, carrierSrcL, numSamples);
    juce::FloatVectorOperations::copy(dryR, carrierSrcR, numSamples);

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
    const ProcessingMode mode = getProcessingMode();

    if (mode != previousMode_)
    {
        resetEnvelopes();
        unvoicedEnv_ = 0.0f;
        std::fill(spectralRingCarrierL_.begin(), spectralRingCarrierL_.end(), 0.0f);
        std::fill(spectralRingCarrierR_.begin(), spectralRingCarrierR_.end(), 0.0f);
        std::fill(spectralRingMod_.begin(), spectralRingMod_.end(), 0.0f);
        std::fill(spectralOutAccumL_.begin(), spectralOutAccumL_.end(), 0.0f);
        std::fill(spectralOutAccumR_.begin(), spectralOutAccumR_.end(), 0.0f);
        spectralWritePos_ = 0;
        spectralSamplesSinceFrame_ = 0;
        outputLimiterEnv_ = 0.0f;
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
    const bool modulatorActive = modRms > 0.0010f;

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

    if (mode == ProcessingMode::Filterbank)
    {
        configureBandFilters(activeBands_, pitchScale, focus, formantFollower, reality);

        const float attackMs = juce::jmap(glue, 0.5f, 14.0f);
        const float releaseMs = juce::jmap(glue, 24.0f, 220.0f);
        const float attackCoeff = computeEnvelopeCoeff(attackMs, sampleRateHz_);
        const float releaseCoeff = computeEnvelopeCoeff(releaseMs, sampleRateHz_);
        const float detectorBlend = 0.25f + 0.35f * focus;
        const float denom = static_cast<float>(juce::jmax(1, activeBands_ - 1));
        std::array<float, kMaxBands> envelopeSnapshot{};
        std::array<float, kMaxBands> staticBandGain{};
        const float envelopeShape = 1.10f + 1.65f * focus;
        const float bandNormalization = 0.72f / std::sqrt(static_cast<float>(activeBands_));

        for (int b = 0; b < activeBands_; ++b)
        {
            const float position = static_cast<float>(b) / denom;
            const float tiltDb = formantTilt * (position - 0.5f) * 10.0f;
            const float tiltGain = juce::Decibels::decibelsToGain(tiltDb);
            const float focusBoost = 1.0f + focus * (1.0f - std::abs(2.0f * position - 1.0f)) * 0.20f;
            staticBandGain[static_cast<size_t>(b)] = bandNormalization * tiltGain * focusBoost;
        }

        for (int s = 0; s < numSamples; ++s)
        {
            const float detectorInput = modMono[s] + detectorBlend * modAnalysis[s];
            float envelopeMean = 0.0f;

            for (int b = 0; b < activeBands_; ++b)
            {
                auto& band = bands_[static_cast<size_t>(b)];
                float env = band.envelope;

                const float detector = std::abs(band.modFilter.processSample(0, detectorInput));
                const float coeff = (detector > env) ? attackCoeff : releaseCoeff;
                env += coeff * (detector - env);
                band.envelope = env;

                envelopeSnapshot[static_cast<size_t>(b)] = env;
                envelopeMean += env;
            }

            envelopeMean = envelopeMean / static_cast<float>(juce::jmax(1, activeBands_)) + 1.0e-8f;

            float wetSampleL = 0.0f;
            float wetSampleR = 0.0f;
            for (int b = 0; b < activeBands_; ++b)
            {
                auto& band = bands_[static_cast<size_t>(b)];

                float relativeEnvelope = envelopeSnapshot[static_cast<size_t>(b)] / envelopeMean;
                relativeEnvelope = juce::jlimit(0.0f, 2.5f, relativeEnvelope);
                relativeEnvelope = std::pow(relativeEnvelope, envelopeShape);

                if (!modulatorActive)
                    relativeEnvelope = 0.0f;

                if (experimentalAmount > 0.0f)
                    relativeEnvelope = std::pow(relativeEnvelope, 1.0f - 0.15f * experimentalAmount);

                const float morphGain = juce::jmap(merge, 0.015f, relativeEnvelope);
                const float bandGain = staticBandGain[static_cast<size_t>(b)] * morphGain;

                wetSampleL += band.carFilter.processSample(0, carrierL[s]) * bandGain;
                wetSampleR += band.carFilter.processSample(1, carrierR[s]) * bandGain;
            }

            wetL[s] = wetSampleL;
            wetR[s] = wetSampleR;
        }
    }
    else
    {
        ensureSpectralSetup(numSamples);

        const int fftSize = spectralSize_;
        const int halfBins = fftSize / 2;
        const float invFft = 1.0f / static_cast<float>(fftSize);
        const float detectorBlend = 0.20f + 0.30f * focus;
        const float formantTiltScale = formantFollower * 7.0f;
        const float focusExponent = juce::jmap(focus, 0.85f, 1.30f);

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

            double modMagSum = 0.0;
            double carrierMagSum = 0.0;
            for (int k = 0; k <= halfBins; ++k)
            {
                const float modMag = std::abs(spectralModFreq_[static_cast<size_t>(k)]);
                spectralModMag_[static_cast<size_t>(k)] = modMag;
                modMagSum += static_cast<double>(modMag);
                carrierMagSum += static_cast<double>(std::abs(spectralCarrierFreqL_[static_cast<size_t>(k)]));
                carrierMagSum += static_cast<double>(std::abs(spectralCarrierFreqR_[static_cast<size_t>(k)]));
            }

            const int smoothingRadius = juce::jlimit(2, 20, juce::roundToInt(juce::jmap(1.0f - focus, 5.0f, 18.0f)));
            for (int k = 0; k <= halfBins; ++k)
            {
                float smoothed = 0.0f;
                float weightSum = 0.0f;
                for (int r = -smoothingRadius; r <= smoothingRadius; ++r)
                {
                    const int idx = juce::jlimit(0, halfBins, k + r);
                    const float weight = 1.0f - (std::abs(static_cast<float>(r)) / static_cast<float>(smoothingRadius + 1));
                    smoothed += spectralModMag_[static_cast<size_t>(idx)] * weight;
                    weightSum += weight;
                }
                spectralModMagSmoothed_[static_cast<size_t>(k)] = smoothed / juce::jmax(1.0e-6f, weightSum);
            }

            const float magnitudeScale = modulatorActive
                ? static_cast<float>((carrierMagSum / (2.0 * (modMagSum + 1.0e-9))) * 0.62)
                : 0.0f;
            const float inactiveDamp = 1.0f - merge * 0.95f;

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

                        float targetMag = std::pow(juce::jmax(0.0f, mMag * magnitudeScale), focusExponent);
                        targetMag = juce::jlimit(0.0f, carrierMag * 2.5f, targetMag);

                        const float position = static_cast<float>(k) / static_cast<float>(juce::jmax(1, halfBins));
                        const float tiltDb = (position - 0.5f) * formantTiltScale;
                        targetMag *= juce::Decibels::decibelsToGain(tiltDb);

                        outMag = juce::jmap(merge, carrierMag, targetMag);
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
                    spectralCarrierTimeL_[static_cast<size_t>(i)].real() * invFft * window;
                spectralOutAccumR_[static_cast<size_t>(ringPos)] +=
                    spectralCarrierTimeR_[static_cast<size_t>(i)].real() * invFft * window;
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
            }

            wetL[s] = spectralOutAccumL_[static_cast<size_t>(spectralWritePos_)];
            wetR[s] = spectralOutAccumR_[static_cast<size_t>(spectralWritePos_)];
            spectralOutAccumL_[static_cast<size_t>(spectralWritePos_)] = 0.0f;
            spectralOutAccumR_[static_cast<size_t>(spectralWritePos_)] = 0.0f;

            spectralWritePos_ = (spectralWritePos_ + 1) % fftSize;
        }
    }

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
    const float sibilanceBaseGain = (0.0003f + 0.018f * sibilance)
                                  * (0.70f + 0.30f * focus)
                                  * (1.0f + 0.30f * experimentalAmount);
    const float limiterAttackCoeff = computeEnvelopeCoeff(0.12f, sampleRateHz_);
    const float limiterReleaseCoeff = computeEnvelopeCoeff(90.0f, sampleRateHz_);
    constexpr float limiterThreshold = 0.92f;

    float unvoicedEnv = unvoicedEnv_;
    float limiterEnv = outputLimiterEnv_;

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
        }

        outL[s] = outSampleL;
        if (stereoOutput)
            outR[s] = outSampleR;
    }

    unvoicedEnv_ = unvoicedEnv;
    outputLimiterEnv_ = limiterEnv;
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
