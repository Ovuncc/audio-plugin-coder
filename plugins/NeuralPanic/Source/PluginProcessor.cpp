#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

NeuralPanicAudioProcessor::NeuralPanicAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
    #if !JucePlugin_IsMidiEffect
     #if !JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
     #endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    #endif
    )
#endif
    , apvts_(*this, nullptr, "NeuralPanicParameters", createParameterLayout())
{
    meltdownSaturator_.functionToUse = [](float x)
    {
        return std::tanh(x);
    };
}

NeuralPanicAudioProcessor::~NeuralPanicAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout NeuralPanicAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto percentText = [](float value, int) { return juce::String(juce::roundToInt(value * 100.0f)) + "%"; };

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::mode, 1),
        "Mode",
        juce::StringArray{ "Agent", "Dropout", "Collapse" },
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::panic, 1),
        "Panic",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.45f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::bandwidth, 1),
        "Bandwidth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.62f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::collapse, 1),
        "Latent Collapse",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.30f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::jitter, 1),
        "Jitter",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.25f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::pitchDrift, 1),
        "Pitch Drift",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.35f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::formantMelt, 1),
        "Formant Melt",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.40f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::broadcast, 1),
        "Broadcast Panic",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.25f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::smear, 1),
        "Temporal Smear",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.35f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID(ParameterIDs::seed, 1),
        "Chaos Seed",
        0,
        9999,
        1337));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::mix, 1),
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        1.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        percentText));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::outputGain, 1),
        "Output",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::bypass, 1),
        "Bypass",
        false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::codecBypass, 1),
        "Codec Bypass",
        false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::packetBypass, 1),
        "Packet Bypass",
        false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::identityBypass, 1),
        "Identity Bypass",
        false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::dynamicsBypass, 1),
        "Dynamics Bypass",
        false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::temporalBypass, 1),
        "Temporal Bypass",
        false));

    return layout;
}

const juce::String NeuralPanicAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NeuralPanicAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NeuralPanicAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NeuralPanicAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NeuralPanicAudioProcessor::getTailLengthSeconds() const
{
    return 0.20;
}

int NeuralPanicAudioProcessor::getNumPrograms()
{
    return 1;
}

int NeuralPanicAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NeuralPanicAudioProcessor::setCurrentProgram(int)
{
}

const juce::String NeuralPanicAudioProcessor::getProgramName(int)
{
    return {};
}

void NeuralPanicAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void NeuralPanicAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleRateHz_ = sampleRate;
    maximumBlockSize_ = juce::jmax(32, samplesPerBlock);

    dryBuffer_.setSize(2, maximumBlockSize_);
    wetBuffer_.setSize(2, maximumBlockSize_);
    gateBuffer_.assign(static_cast<size_t>(maximumBlockSize_), 0.0f);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRateHz_;
    spec.maximumBlockSize = static_cast<juce::uint32>(maximumBlockSize_);
    spec.numChannels = 2;

    meltdownCompressor_.prepare(spec);
    meltdownCompressor_.setAttack(0.8f);
    meltdownCompressor_.setRelease(120.0f);
    meltdownCompressor_.setRatio(4.0f);
    meltdownCompressor_.setThreshold(-14.0f);

    meltdownSaturator_.prepare(spec);

    outputLimiter_.prepare(spec);
    outputLimiter_.setRelease(80.0f);
    outputLimiter_.setThreshold(-0.5f);

    panicSmoother_.reset(sampleRateHz_, 0.04);
    panicSmoother_.setCurrentAndTargetValue(apvts_.getRawParameterValue(ParameterIDs::panic)->load());

    mixSmoother_.reset(sampleRateHz_, 0.02);
    mixSmoother_.setCurrentAndTargetValue(apvts_.getRawParameterValue(ParameterIDs::mix)->load());

    outputGainSmoother_.reset(sampleRateHz_, 0.02);
    outputGainSmoother_.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(apvts_.getRawParameterValue(ParameterIDs::outputGain)->load()));

    broadcastSmoother_.reset(sampleRateHz_, 0.04);
    broadcastSmoother_.setCurrentAndTargetValue(apvts_.getRawParameterValue(ParameterIDs::broadcast)->load());

    smearSmoother_.reset(sampleRateHz_, 0.04);
    smearSmoother_.setCurrentAndTargetValue(apvts_.getRawParameterValue(ParameterIDs::smear)->load());

    jitterDelaySize_ = juce::jmax(256, juce::roundToInt(static_cast<float>(sampleRateHz_ * 0.08)) + maximumBlockSize_ + 8);
    for (auto& delay : jitterDelay_)
        delay.assign(static_cast<size_t>(jitterDelaySize_), 0.0f);

    smearBufferSize_ = juce::jmax(512, juce::roundToInt(static_cast<float>(sampleRateHz_ * 0.25)) + maximumBlockSize_ + 8);
    for (auto& delay : smearBuffer_)
        delay.assign(static_cast<size_t>(smearBufferSize_), 0.0f);

    gateHoldSamples_ = juce::jmax(1, juce::roundToInt(static_cast<float>(sampleRateHz_ * 0.06)));
    gateAttackCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRateHz_ * 0.0025));
    gateReleaseCoeff_ = std::exp(-1.0f / static_cast<float>(sampleRateHz_ * 0.05));
    silenceResetSamples_ = juce::jmax(1, juce::roundToInt(static_cast<float>(sampleRateHz_ * 0.14)));

    updateRandomSeed(static_cast<int>(std::lround(apvts_.getRawParameterValue(ParameterIDs::seed)->load())));
    resetInternalState();
}

void NeuralPanicAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NeuralPanicAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if !JucePlugin_IsSynth
    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void NeuralPanicAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int totalInputChannels = getTotalNumInputChannels();
    const int totalOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalInputChannels; ch < totalOutputChannels; ++ch)
        buffer.clear(ch, 0, numSamples);

    if (numSamples <= 0)
        return;

    if (apvts_.getRawParameterValue(ParameterIDs::bypass)->load() > 0.5f)
        return;

    if (numSamples > dryBuffer_.getNumSamples())
    {
        dryBuffer_.setSize(2, numSamples, false, false, true);
        wetBuffer_.setSize(2, numSamples, false, false, true);
    }
    if (numSamples > static_cast<int>(gateBuffer_.size()))
        gateBuffer_.assign(static_cast<size_t>(numSamples), 0.0f);
    else
        std::fill(gateBuffer_.begin(), gateBuffer_.begin() + numSamples, 0.0f);

    const float* inL = buffer.getReadPointer(0);
    const float* inR = buffer.getReadPointer(totalInputChannels > 1 ? 1 : 0);
    float* dryL = dryBuffer_.getWritePointer(0);
    float* dryR = dryBuffer_.getWritePointer(1);
    float* wetL = wetBuffer_.getWritePointer(0);
    float* wetR = wetBuffer_.getWritePointer(1);

    double blockEnergy = 0.0;
    double blockDelta = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        dryL[i] = inL[i];
        dryR[i] = inR[i];
        wetL[i] = 0.0f;
        wetR[i] = 0.0f;

        const float mono = 0.5f * (inL[i] + inR[i]);
        blockEnergy += static_cast<double>(mono * mono);
        blockDelta += static_cast<double>(std::abs(mono - inputPrevSample_));
        inputPrevSample_ = mono;
    }

    const float blockRms = std::sqrt(static_cast<float>(blockEnergy / static_cast<double>(numSamples)));
    const float blockDeltaMean = static_cast<float>(blockDelta / static_cast<double>(numSamples));

    inputRmsSmoothed_ += 0.12f * (blockRms - inputRmsSmoothed_);
    inputDeltaSmoothed_ += 0.20f * (blockDeltaMean - inputDeltaSmoothed_);

    const float energyActivity = juce::jlimit(0.0f, 1.0f, juce::jmap(inputRmsSmoothed_, 0.00035f, 0.08f, 0.0f, 1.0f));
    const float motionActivity = juce::jlimit(0.0f, 1.0f, juce::jmap(inputDeltaSmoothed_, 0.00055f, 0.04f, 0.0f, 1.0f));

    const float activityTarget = juce::jlimit(0.0f, 1.0f, energyActivity * 0.76f + motionActivity * 0.24f);
    inputActivity_ += 0.16f * (activityTarget - inputActivity_);
    inputMotion_ += 0.20f * (motionActivity - inputMotion_);

    const float voiceActivityBlock = juce::jlimit(0.0f, 1.0f, inputActivity_);
    const float voiceMotionBlock = juce::jlimit(0.0f, 1.0f, inputMotion_ + std::abs(voiceActivityBlock - energyActivity) * 0.85f);

    if (inputRmsSmoothed_ < juce::Decibels::decibelsToGain(-60.0f) && voiceActivityBlock < 0.02f)
        silenceSampleCounter_ += numSamples;
    else
        silenceSampleCounter_ = 0;

    const bool deepSilence = silenceSampleCounter_ >= silenceResetSamples_;
    if (deepSilence && !deepSilenceLatched_)
    {
        // Hard-reset stochastic state after sustained silence to avoid idle artifacts.
        resetInternalState();
        silenceSampleCounter_ = silenceResetSamples_;
        deepSilenceLatched_ = true;
    }
    else if (!deepSilence)
    {
        deepSilenceLatched_ = false;
    }

    const bool gateOpenTrigger = ((inputRmsSmoothed_ > juce::Decibels::decibelsToGain(-50.0f)) && motionActivity > 0.06f)
                              || voiceActivityBlock > 0.16f;
    const bool gateCloseTrigger = (inputRmsSmoothed_ < juce::Decibels::decibelsToGain(-58.0f))
                               || (voiceActivityBlock < 0.05f && motionActivity < 0.04f)
                               || deepSilence;
    if (gateOpenTrigger)
    {
        gateOpenState_ = true;
        gateHoldCounter_ = gateHoldSamples_;
    }
    else
    {
        if (gateHoldCounter_ > 0)
            --gateHoldCounter_;

        if (gateCloseTrigger && gateHoldCounter_ == 0)
            gateOpenState_ = false;
    }
    const float gateTarget = (deepSilence ? 0.0f : (gateOpenState_ ? 1.0f : 0.0f));

    auto* modeParam = dynamic_cast<juce::AudioParameterChoice*>(apvts_.getParameter(ParameterIDs::mode));
    const int mode = (modeParam != nullptr) ? modeParam->getIndex()
                                            : juce::roundToInt(apvts_.getRawParameterValue(ParameterIDs::mode)->load());

    const float bandwidth = juce::jlimit(0.0f, 1.0f, apvts_.getRawParameterValue(ParameterIDs::bandwidth)->load());
    const float collapse = juce::jlimit(0.0f, 1.0f, apvts_.getRawParameterValue(ParameterIDs::collapse)->load());
    const float jitter = juce::jlimit(0.0f, 1.0f, apvts_.getRawParameterValue(ParameterIDs::jitter)->load());
    const float pitchDrift = juce::jlimit(0.0f, 1.0f, apvts_.getRawParameterValue(ParameterIDs::pitchDrift)->load());
    const float formantMelt = juce::jlimit(0.0f, 1.0f, apvts_.getRawParameterValue(ParameterIDs::formantMelt)->load());
    const bool codecBypassed = apvts_.getRawParameterValue(ParameterIDs::codecBypass)->load() > 0.5f;
    const bool packetBypassed = apvts_.getRawParameterValue(ParameterIDs::packetBypass)->load() > 0.5f;
    const bool identityBypassed = apvts_.getRawParameterValue(ParameterIDs::identityBypass)->load() > 0.5f;
    const bool dynamicsBypassed = apvts_.getRawParameterValue(ParameterIDs::dynamicsBypass)->load() > 0.5f;
    const bool temporalBypassed = apvts_.getRawParameterValue(ParameterIDs::temporalBypass)->load() > 0.5f;

    panicSmoother_.setTargetValue(juce::jlimit(0.0f, 1.0f, apvts_.getRawParameterValue(ParameterIDs::panic)->load()));
    mixSmoother_.setTargetValue(juce::jlimit(0.0f, 1.0f, apvts_.getRawParameterValue(ParameterIDs::mix)->load()));
    outputGainSmoother_.setTargetValue(
        juce::Decibels::decibelsToGain(apvts_.getRawParameterValue(ParameterIDs::outputGain)->load()));
    broadcastSmoother_.setTargetValue(juce::jlimit(0.0f, 1.0f, apvts_.getRawParameterValue(ParameterIDs::broadcast)->load()));
    smearSmoother_.setTargetValue(juce::jlimit(0.0f, 1.0f, apvts_.getRawParameterValue(ParameterIDs::smear)->load()));

    const float engineEnable = deepSilence ? 0.0f : 1.0f;
    const float codecEnable = codecBypassed ? 0.0f : 1.0f;
    const float packetEnable = packetBypassed ? 0.0f : 1.0f;
    const float identityEnable = identityBypassed ? 0.0f : 1.0f;
    const float dynamicsEnable = dynamicsBypassed ? 0.0f : 1.0f;
    const float temporalEnable = temporalBypassed ? 0.0f : 1.0f;
    const float modeBias = (mode == 2 ? 0.16f : mode == 1 ? 0.08f : 0.0f);

    const float activityScale = engineEnable * juce::jlimit(0.0f, 1.0f, 0.12f + 0.88f * voiceActivityBlock);
    const float motionScale = engineEnable * juce::jlimit(0.0f, 1.0f, 0.20f + 0.80f * voiceMotionBlock);
    const float panicKnob = panicSmoother_.getTargetValue();
    const float panicBlock = juce::jlimit(0.0f, 1.0f,
                                          panicKnob * (0.30f + 0.70f * activityScale)
                                          + panicKnob * panicKnob * 0.25f
                                          + modeBias * (0.16f + 0.20f * activityScale));
    const float bandwidthDynamic = juce::jlimit(0.0f, 1.0f,
                                                bandwidth
                                                - codecEnable * panicBlock * (0.10f + 0.25f * motionScale)
                                                + codecEnable * (voiceMotionBlock - 0.5f) * (0.09f + panicBlock * 0.14f));
    const float collapseAmount = juce::jlimit(0.0f, 1.0f,
                                              codecEnable * (collapse * (0.35f + 0.65f * activityScale)
                                                           + panicBlock * (0.45f + 0.55f * motionScale)
                                                           + modeBias * (0.10f + 0.12f * motionScale)));
    const float jitterActive = juce::jlimit(0.0f, 1.0f,
                                            packetEnable * (jitter * (0.30f + 0.70f * activityScale) * (0.45f + 0.55f * motionScale)
                                                          + panicBlock * 0.10f
                                                          + modeBias * (0.14f + 0.10f * motionScale)));
    const float pitchDriftActive = juce::jlimit(0.0f, 1.0f,
                                                identityEnable * (pitchDrift * (0.30f + 0.70f * activityScale)
                                                                + panicBlock * 0.20f
                                                                + modeBias * (0.18f + 0.15f * motionScale)));
    const float formantMeltActive = juce::jlimit(0.0f, 1.0f,
                                                 identityEnable * (formantMelt * (0.30f + 0.70f * activityScale)
                                                                 + panicBlock * 0.12f
                                                                 + modeBias * (0.12f + 0.10f * activityScale)));

    const int downsampleFactor = codecBypassed
        ? 1
        : juce::jlimit(1, 10, 1 + juce::roundToInt((1.0f - bandwidthDynamic) * (4.0f + panicBlock * 6.2f + motionScale * 2.8f)));
    const int bitDepth = codecBypassed
        ? 12
        : juce::jlimit(3, 12, 12 - juce::roundToInt((1.0f - bandwidthDynamic) * 5.2f + collapseAmount * 4.2f + motionScale * 1.8f + panicBlock * 2.0f));
    const float quantLevels = static_cast<float>(1 << bitDepth);
    const float quantStep = 2.0f / juce::jmax(2.0f, quantLevels - 1.0f);

    const float dynamicsAmount = juce::jlimit(0.0f, 1.0f, dynamicsEnable * (broadcastSmoother_.getTargetValue() * activityScale + panicBlock * 0.55f));
    meltdownCompressor_.setThreshold(-8.0f - dynamicsAmount * 28.0f);
    meltdownCompressor_.setRatio(2.0f + dynamicsAmount * 10.0f);
    meltdownCompressor_.setAttack(0.3f + (1.0f - dynamicsAmount) * 1.4f);
    meltdownCompressor_.setRelease(35.0f + (1.0f - dynamicsAmount) * 120.0f);

    outputLimiter_.setThreshold(-0.3f - panicBlock * 1.8f);
    outputLimiter_.setRelease(80.0f);

    saturatorDrive_ = 1.0f + dynamicsAmount * 4.8f;

    const int seedValue = juce::jlimit(0, 9999, juce::roundToInt(apvts_.getRawParameterValue(ParameterIDs::seed)->load()));
    updateRandomSeed(seedValue);

    for (int s = 0; s < numSamples; ++s)
    {
        const float panic = panicSmoother_.getNextValue() * (0.20f + 0.80f * voiceActivityBlock);
        const float panicJolt = panic * (0.55f + 0.45f * std::abs(randomSigned()));
        const float gateCoeff = (gateTarget > gateEnvelope_) ? gateAttackCoeff_ : gateReleaseCoeff_;
        gateEnvelope_ = gateTarget + gateCoeff * (gateEnvelope_ - gateTarget);
        const float gateNow = gateEnvelope_;
        gateBuffer_[static_cast<size_t>(s)] = gateNow;
        const float motionChaos = juce::jlimit(0.0f, 1.0f,
                                               motionScale * (0.55f + 0.45f * std::abs(randomSigned()))
                                               + panicJolt * 0.35f);

        if (!packetBypassed && --jitterUpdateCounter_ <= 0)
        {
            const float jitterGate = gateNow * (0.25f + 0.75f * voiceActivityBlock);
            const float maxJitterMs = (0.8f + jitterActive * 46.0f + panicJolt * 12.0f + modeBias * 16.0f) * jitterGate;
            const float newOffset = random01_(rng_) * maxJitterMs * 0.001f * static_cast<float>(sampleRateHz_);
            jitterOffsetTarget_ = newOffset * (0.25f + 0.75f * gateNow + 0.20f * voiceMotionBlock);
            jitterUpdateCounter_ = 8 + juce::roundToInt(random01_(rng_) * juce::jmax(8.0f, 72.0f - voiceMotionBlock * 28.0f - panicJolt * 18.0f));
        }
        if (!packetBypassed)
        {
            const float tracking = 0.02f + jitterActive * 0.06f;
            jitterOffsetCurrent_ += tracking * (jitterOffsetTarget_ - jitterOffsetCurrent_);

            if (gateNow < 0.02f)
                jitterOffsetCurrent_ *= 0.92f;
        }
        else
        {
            jitterOffsetCurrent_ = 0.0f;
            jitterOffsetTarget_ = 0.0f;
        }

        if (!identityBypassed && --driftRandomCounter_ <= 0)
        {
            driftRandom_ = randomSigned() * (0.65f + panicJolt * 1.00f + modeBias * 0.80f);
            driftRandomCounter_ = 6 + juce::roundToInt(random01_(rng_) * juce::jmax(8.0f, 44.0f - voiceMotionBlock * 18.0f - panicJolt * 16.0f));
        }

        if (!identityBypassed)
        {
            driftPhase_ += juce::MathConstants<double>::twoPi * (2.0 + (pitchDriftActive + panicJolt) * 16.0) / sampleRateHz_;
            if (driftPhase_ >= juce::MathConstants<double>::twoPi)
                driftPhase_ -= juce::MathConstants<double>::twoPi;

            if (--identityEventCounter_ <= 0)
            {
                const float eventChance = (0.0004f + pitchDriftActive * 0.0032f + formantMeltActive * 0.0030f
                                         + motionChaos * 0.0022f + modeBias * 0.0015f) * gateNow;
                if (random01_(rng_) < eventChance)
                    identityEventDepth_ = juce::jlimit(0.0f, 1.0f, identityEventDepth_ + 0.25f + random01_(rng_) * 0.70f);

                identityEventCounter_ = 6 + juce::roundToInt(random01_(rng_) * juce::jmax(6.0f, 26.0f - voiceMotionBlock * 10.0f - panicJolt * 8.0f));
            }

            const float decay = (gateNow < 0.08f) ? 0.88f : 0.96f;
            identityEventDepth_ *= decay;
            if (identityEventDepth_ < 1.0e-4f)
                identityEventDepth_ = 0.0f;
        }
        else
        {
            identityEventDepth_ = 0.0f;
            identityEventCounter_ = 0;
        }

        const float driftStrength = identityBypassed
            ? 0.0f
            : (pitchDriftActive * (0.45f + 0.55f * voiceActivityBlock)
               + panicJolt * 0.55f
               + modeBias * 0.25f) * gateNow;
        const float driftOsc = identityBypassed ? 0.0f : std::sin(static_cast<float>(driftPhase_) + driftRandom_) * driftStrength;
        const float broadcastSample = dynamicsEnable * (broadcastSmoother_.getNextValue() * (0.12f + 0.88f * voiceActivityBlock) + panicJolt * 0.25f);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float src = (ch == 0 ? inL[s] : inR[s]);
            float stage = src;

            if (!codecBypassed)
            {
                if (decimCounter_[ch] == 0)
                    heldSample_[ch] = src;
                decimCounter_[ch] = (decimCounter_[ch] + 1) % downsampleFactor;

                float codec = heldSample_[ch];
                codec = std::round((codec + 1.0f) / quantStep) * quantStep - 1.0f;
                codec = juce::jlimit(-1.0f, 1.0f, codec);

                const float corruptionChance = collapseAmount * (0.0005f + panicJolt * 0.0060f) * (0.35f + 0.65f * motionChaos) * gateNow;
                if (random01_(rng_) < corruptionChance)
                {
                    const float shape = random01_(rng_);
                    if (shape < 0.33f)
                        codec = -codec * (0.7f + 0.3f * random01_(rng_));
                    else if (shape < 0.66f)
                        codec = std::copysign(0.85f, codec);
                    else
                        codec = codec * 0.2f + randomSigned() * 0.18f;
                }

                if (gateNow > 0.1f && random01_(rng_) < panicJolt * 0.02f)
                    codec += randomSigned() * (0.03f + 0.10f * motionChaos);

                stage = juce::jlimit(-1.0f, 1.0f, codec);
            }

            if (!packetBypassed)
            {
                jitterDelay_[ch][static_cast<size_t>(jitterWritePos_)] = stage;
                stage = readCircularInterpolated(jitterDelay_[ch], jitterWritePos_, jitterOffsetCurrent_);
            }

            float unstable = stage;
            if (!identityBypassed)
            {
                const float formantDepth = juce::jlimit(0.0f, 1.0f, formantMeltActive + panicJolt * 0.35f + identityEventDepth_ * 0.25f);
                const float driftDepth = juce::jlimit(0.0f, 1.0f, pitchDriftActive + panicJolt * 0.35f + identityEventDepth_ * 0.30f);
                const float delta = unstable - identityPrev_[ch];
                const float onset = juce::jlimit(0.0f, 1.0f, juce::jmap(std::abs(delta), 0.0015f, 0.08f, 0.0f, 1.0f));

                identityColor_[ch] += 0.04f * ((randomSigned() * (0.20f + driftDepth * 0.80f) * gateNow) - identityColor_[ch]);
                const float color = identityColor_[ch];

                const float microWarp = driftOsc * (0.35f + driftDepth * 0.80f) + color * 0.12f * driftDepth;
                unstable = unstable * (1.0f + microWarp);
                unstable += delta * (0.18f + driftDepth * 0.55f);

                const float lowCoeff = juce::jlimit(0.01f, 0.45f, 0.03f + (1.0f - formantDepth) * 0.20f - identityEventDepth_ * 0.012f);
                lowState_[ch] += lowCoeff * (unstable - lowState_[ch]);

                const float highIn = unstable - lowState_[ch];
                const float highCoeff = juce::jlimit(0.08f, 0.48f, 0.10f + formantDepth * 0.30f + onset * 0.06f);
                highState_[ch] += highCoeff * (highIn - highState_[ch]);

                const float voicedSwap = juce::jlimit(0.0f, 1.0f, driftDepth * 0.45f + onset * 0.35f + identityEventDepth_ * 0.45f);
                const float swapped = 0.56f * unstable + 0.44f * identityPrev_[ch] * (0.75f + 0.25f * color);
                unstable = juce::jmap(voicedSwap, unstable, swapped);

                const float tilt = 0.40f + formantDepth * 1.95f + std::abs(color) * 0.65f + identityEventDepth_ * 0.85f;
                unstable = lowState_[ch] * (1.0f - formantDepth * 0.88f)
                         + highState_[ch] * tilt;

                const float morphChance = (0.0004f + formantDepth * 0.0036f + driftDepth * 0.0021f
                                         + onset * 0.0018f + identityEventDepth_ * 0.0048f) * gateNow;
                if (random01_(rng_) < morphChance)
                {
                    if (random01_(rng_) < 0.5f)
                        unstable = std::tanh((unstable + identityPrev_[ch] * 0.65f) * (2.0f + formantDepth * 4.0f));
                    else
                        unstable = std::copysign(std::sqrt(std::abs(unstable)), unstable) * (0.65f + driftDepth * 0.70f);
                }

                unstable = juce::jlimit(-2.0f, 2.0f, unstable);
                identityPrev_[ch] = unstable;
            }
            else
            {
                identityPrev_[ch] = stage;
                identityColor_[ch] *= 0.95f;
            }

            if (!dynamicsBypassed)
            {
                unstable += randomSigned() * panicJolt * motionChaos * 0.06f * gateNow;
                const float preDrive = 1.0f + broadcastSample * 3.6f + panicJolt * 2.5f + motionChaos * 0.7f;
                unstable = std::tanh(unstable * preDrive);
            }

            unstable *= gateNow;

            if (ch == 0)
                wetL[s] = unstable;
            else
                wetR[s] = unstable;
        }

        if (!packetBypassed)
        {
            ++jitterWritePos_;
            if (jitterWritePos_ >= jitterDelaySize_)
                jitterWritePos_ = 0;
        }
    }

    juce::dsp::AudioBlock<float> wetBlock(wetBuffer_);
    juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);
    if (!dynamicsBypassed)
    {
        meltdownCompressor_.process(wetContext);

        for (int s = 0; s < numSamples; ++s)
        {
            wetL[s] *= saturatorDrive_;
            wetR[s] *= saturatorDrive_;
        }

        meltdownSaturator_.process(wetContext);
    }
    outputLimiter_.process(wetContext);

    for (int s = 0; s < numSamples; ++s)
    {
        const float panic = panicSmoother_.getCurrentValue() * (0.20f + 0.80f * voiceActivityBlock);
        const float smearAmount = smearSmoother_.getNextValue() * temporalEnable;
        const float gateNow = gateBuffer_[static_cast<size_t>(s)];

        if (!temporalBypassed)
        {
            if (--smearRandomCounter_ <= 0)
            {
                const float baseMs = 30.0f + smearAmount * 90.0f;
                const float modMs = random01_(rng_) * (10.0f + panic * 32.0f + voiceMotionBlock * 20.0f);
                smearReadOffset_ = (baseMs + modMs) * 0.001f * static_cast<float>(sampleRateHz_);
                smearRandomCounter_ = 6 + juce::roundToInt(random01_(rng_) * juce::jmax(8.0f, 20.0f - voiceMotionBlock * 8.0f - panic * 6.0f));
            }

            float delayedL = readCircularInterpolated(smearBuffer_[0], smearWritePos_, smearReadOffset_);
            float delayedR = readCircularInterpolated(smearBuffer_[1], smearWritePos_, smearReadOffset_);

            if (smearFreezeCounter_ > 0)
            {
                delayedL = smearFrozen_[0];
                delayedR = smearFrozen_[1];
            }
            else
            {
                const float freezeChance = smearAmount * (0.0007f + panic * 0.0030f) * gateNow * (0.4f + 0.6f * voiceMotionBlock);
                if (random01_(rng_) < freezeChance)
                {
                    smearFrozen_[0] = delayedL;
                    smearFrozen_[1] = delayedR;
                    smearFreezeCounter_ = 24 + juce::roundToInt(random01_(rng_) * (40.0f + smearAmount * 120.0f));
                }
            }

            const float smearBlend = juce::jlimit(0.0f, 1.0f, smearAmount * (0.40f + panic * 0.80f) * gateNow);
            const float feedback = (0.08f + smearAmount * 0.45f + panic * 0.18f) * (0.20f + 0.80f * gateNow);

            const float sourceL = wetL[s];
            const float sourceR = wetR[s];
            const float mixedL = juce::jmap(smearBlend, sourceL, delayedL);
            const float mixedR = juce::jmap(smearBlend, sourceR, delayedR);
            const float writeScale = 0.20f + gateNow * 0.80f;

            smearBuffer_[0][static_cast<size_t>(smearWritePos_)] = juce::jlimit(-1.2f, 1.2f, (sourceL + delayedL * feedback) * writeScale);
            smearBuffer_[1][static_cast<size_t>(smearWritePos_)] = juce::jlimit(-1.2f, 1.2f, (sourceR + delayedR * feedback) * writeScale);

            wetL[s] = mixedL * gateNow;
            wetR[s] = mixedR * gateNow;

            if (smearFreezeCounter_ > 0)
                --smearFreezeCounter_;

            ++smearWritePos_;
            if (smearWritePos_ >= smearBufferSize_)
                smearWritePos_ = 0;
        }
        else
        {
            wetL[s] *= gateNow * gateNow;
            wetR[s] *= gateNow * gateNow;
        }

        if (deepSilence || gateNow < 0.0008f)
        {
            wetL[s] = 0.0f;
            wetR[s] = 0.0f;
        }
    }

    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getWritePointer(totalOutputChannels > 1 ? 1 : 0);

    for (int s = 0; s < numSamples; ++s)
    {
        const float mix = mixSmoother_.getNextValue();
        const float outGain = outputGainSmoother_.getNextValue();

        float left = ((1.0f - mix) * dryL[s] + mix * wetL[s]) * outGain;
        float right = ((1.0f - mix) * dryR[s] + mix * wetR[s]) * outGain;

        left = std::tanh(left * 1.05f);
        right = std::tanh(right * 1.05f);

        if (deepSilence)
        {
            if (std::abs(left) < 1.0e-4f)
                left = 0.0f;
            if (std::abs(right) < 1.0e-4f)
                right = 0.0f;
        }

        outL[s] = left;
        if (totalOutputChannels > 1)
            outR[s] = right;
    }
}

bool NeuralPanicAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* NeuralPanicAudioProcessor::createEditor()
{
    return new NeuralPanicAudioProcessorEditor(*this);
}

void NeuralPanicAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NeuralPanicAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(apvts_.state.getType()))
        apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
}

void NeuralPanicAudioProcessor::resetInternalState()
{
    inputPrevSample_ = 0.0f;
    inputRmsSmoothed_ = 0.0f;
    inputDeltaSmoothed_ = 0.0f;
    inputActivity_ = 0.0f;
    inputMotion_ = 0.0f;
    gateOpenState_ = false;
    gateEnvelope_ = 0.0f;
    gateHoldCounter_ = 0;
    silenceSampleCounter_ = 0;

    decimCounter_ = { 0, 0 };
    heldSample_ = { 0.0f, 0.0f };

    jitterWritePos_ = 0;
    jitterOffsetCurrent_ = 0.0f;
    jitterOffsetTarget_ = 0.0f;
    jitterUpdateCounter_ = 0;

    lowState_ = { 0.0f, 0.0f };
    highState_ = { 0.0f, 0.0f };
    driftPhase_ = 0.0;
    driftRandom_ = 0.0f;
    driftRandomCounter_ = 0;
    identityPrev_ = { 0.0f, 0.0f };
    identityColor_ = { 0.0f, 0.0f };
    identityEventCounter_ = 0;
    identityEventDepth_ = 0.0f;

    smearWritePos_ = 0;
    smearReadOffset_ = 0.0f;
    smearRandomCounter_ = 0;
    smearFreezeCounter_ = 0;
    smearFrozen_ = { 0.0f, 0.0f };

    for (auto& delay : jitterDelay_)
        std::fill(delay.begin(), delay.end(), 0.0f);
    for (auto& delay : smearBuffer_)
        std::fill(delay.begin(), delay.end(), 0.0f);
    std::fill(gateBuffer_.begin(), gateBuffer_.end(), 0.0f);
}

void NeuralPanicAudioProcessor::updateRandomSeed(int seedValue)
{
    if (seedValue == currentSeed_)
        return;

    currentSeed_ = seedValue;
    rng_.seed(static_cast<uint32_t>(seedValue) + 0x9e3779b9u);
}

float NeuralPanicAudioProcessor::randomSigned()
{
    return random01_(rng_) * 2.0f - 1.0f;
}

float NeuralPanicAudioProcessor::readCircularInterpolated(const std::vector<float>& buffer, int writePos, float delaySamples) const
{
    if (buffer.empty())
        return 0.0f;

    const float size = static_cast<float>(buffer.size());
    float readPos = static_cast<float>(writePos) - juce::jlimit(0.0f, size - 2.0f, delaySamples);

    while (readPos < 0.0f)
        readPos += size;

    const int i0 = static_cast<int>(readPos) % static_cast<int>(buffer.size());
    const int i1 = (i0 + 1) % static_cast<int>(buffer.size());
    const float frac = readPos - static_cast<float>(i0);

    return buffer[static_cast<size_t>(i0)] + (buffer[static_cast<size_t>(i1)] - buffer[static_cast<size_t>(i0)]) * frac;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeuralPanicAudioProcessor();
}
