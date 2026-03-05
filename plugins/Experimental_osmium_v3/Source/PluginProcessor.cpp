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
    apvts.addParameterListener(ParameterIDs::intensity, this);
    apvts.addParameterListener(ParameterIDs::expProcessingMode, this);

    macroIntensityPending.store(apvts.getRawParameterValue(ParameterIDs::intensity)->load());
    macroModePending.store(juce::roundToInt(apvts.getRawParameterValue(ParameterIDs::expProcessingMode)->load()));
    macroSyncPending.store(true);
    triggerAsyncUpdate();
}

OsmiumAudioProcessor::~OsmiumAudioProcessor()
{
    cancelPendingUpdate();
    apvts.removeParameterListener(ParameterIDs::intensity, this);
    apvts.removeParameterListener(ParameterIDs::expProcessingMode, this);
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
        juce::NormalisableRange<float>(-30.0f, 0.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::bypass, 1),
        "Bypass",
        false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::cleanLowEnd, 1),
        "Clean Low End",
        false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::expManualMode, 1),
        "EXP Manual Mode",
        false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expCrossoverHz, 1),
        "EXP Crossover (Hz)",
        juce::NormalisableRange<float>(220.0f, 600.0f, 1.0f, 0.4f),
        277.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::expOversamplingMode, 1),
        "EXP Oversampling",
        juce::StringArray { "Off", "4x", "8x" },
        1));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::expProcessingMode, 1),
        "EXP Processing Mode",
        juce::StringArray { "Osmium", "Tight", "Chaotic" },
        0));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::expBypassLowTransient, 1),
        "EXP Bypass Low Transient",
        false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::expBypassLowSaturation, 1),
        "EXP Bypass Low Saturation",
        false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::expBypassLowLimiter, 1),
        "EXP Bypass Low Limiter",
        false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::expBypassHighTransient, 1),
        "EXP Bypass High Transient",
        false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::expBypassHighSaturation, 1),
        "EXP Bypass High Saturation",
        false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::expBypassTightness, 1),
        "EXP Bypass Tightness",
        false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::expBypassOutputLimiter, 1),
        "EXP Bypass Output Limiter",
        false));

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
        juce::NormalisableRange<float>(1.0f, 6.0f, 0.01f),
        1.30f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::expLowWaveShaperType, 1),
        "EXP Low Waveshaper",
        juce::StringArray { "Soft Sign", "Sine", "Hard Clip" },
        1));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowFastAttackMs, 1),
        "EXP Low Fast Attack (ms)",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.45f),
        1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowFastReleaseMs, 1),
        "EXP Low Fast Release (ms)",
        juce::NormalisableRange<float>(1.0f, 80.0f, 0.01f, 0.45f),
        15.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowSlowReleaseMs, 1),
        "EXP Low Slow Release (ms)",
        juce::NormalisableRange<float>(20.0f, 400.0f, 0.1f, 0.45f),
        120.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowAttackGainSmoothMs, 1),
        "EXP Low Attack Gain Smooth (ms)",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.45f),
        1.8f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowCompAttackRatio, 1),
        "EXP Low Comp Attack Ratio",
        juce::NormalisableRange<float>(0.05f, 1.0f, 0.001f, 0.45f),
        0.25f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowCompGainSmoothMs, 1),
        "EXP Low Comp Gain Smooth (ms)",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.45f),
        1.6f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowTransientSensitivity, 1),
        "EXP Low Transient Sensitivity",
        juce::NormalisableRange<float>(0.25f, 6.0f, 0.01f, 0.45f),
        2.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowLimiterThresholdDb, 1),
        "EXP Low Limiter Threshold (dB)",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f),
        -0.15f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLowLimiterReleaseMs, 1),
        "EXP Low Limiter Release (ms)",
        juce::NormalisableRange<float>(10.0f, 400.0f, 0.1f, 0.45f),
        180.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighAttackBoostDb, 1),
        "EXP High Attack Boost (dB)",
        juce::NormalisableRange<float>(0.0f, 16.0f, 0.1f),
        1.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighAttackTimeMs, 1),
        "EXP High Attack Time (ms)",
        juce::NormalisableRange<float>(6.0f, 12.7f, 0.1f, 0.45f),
        6.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighPostCompDb, 1),
        "EXP High Post Attack Comp (dB)",
        juce::NormalisableRange<float>(0.0f, 7.5f, 0.1f),
        0.4f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighCompTimeMs, 1),
        "EXP High Compression Time (ms)",
        juce::NormalisableRange<float>(14.0f, 38.1f, 0.1f, 0.4f),
        14.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighDriveDb, 1),
        "EXP High Drive (dB)",
        juce::NormalisableRange<float>(4.0f, 9.5f, 0.1f),
        4.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighSatMix, 1),
        "EXP High Saturation Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.05f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighSatDrive, 1),
        "EXP High Saturation Drive",
        juce::NormalisableRange<float>(1.0f, 9.0f, 0.01f),
        1.24f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighTrimDb, 1),
        "EXP High Band Trim (dB)",
        juce::NormalisableRange<float>(-4.0f, 0.0f, 0.1f),
        -0.1f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::expHighWaveShaperType, 1),
        "EXP High Waveshaper",
        juce::StringArray { "Soft Sign", "Sine", "Hard Clip" },
        1));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighFastAttackMs, 1),
        "EXP High Fast Attack (ms)",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.45f),
        1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighFastReleaseMs, 1),
        "EXP High Fast Release (ms)",
        juce::NormalisableRange<float>(1.0f, 80.0f, 0.01f, 0.45f),
        15.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighSlowReleaseMs, 1),
        "EXP High Slow Release (ms)",
        juce::NormalisableRange<float>(20.0f, 400.0f, 0.1f, 0.45f),
        120.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighAttackGainSmoothMs, 1),
        "EXP High Attack Gain Smooth (ms)",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.45f),
        1.8f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighCompAttackRatio, 1),
        "EXP High Comp Attack Ratio",
        juce::NormalisableRange<float>(0.05f, 1.0f, 0.001f, 0.45f),
        0.25f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighCompGainSmoothMs, 1),
        "EXP High Comp Gain Smooth (ms)",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.45f),
        1.6f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expHighTransientSensitivity, 1),
        "EXP High Transient Sensitivity",
        juce::NormalisableRange<float>(0.25f, 6.0f, 0.01f, 0.45f),
        2.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightSustainDepthDb, 1),
        "EXP Tight Sustain Depth (dB)",
        juce::NormalisableRange<float>(0.0f, 18.0f, 0.1f),
        0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightSustainAttackMs, 1),
        "EXP Tight Sustain Attack (ms)",
        juce::NormalisableRange<float>(0.2f, 30.0f, 0.01f, 0.45f),
        4.0f));
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
        juce::ParameterID(ParameterIDs::expTightHighShelfQ, 1),
        "EXP Tight High Shelf Q",
        juce::NormalisableRange<float>(0.3f, 2.0f, 0.01f, 0.45f),
        0.7071f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightFastAttackMs, 1),
        "EXP Tight Fast Attack (ms)",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.45f),
        1.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightFastReleaseMs, 1),
        "EXP Tight Fast Release (ms)",
        juce::NormalisableRange<float>(1.0f, 120.0f, 0.01f, 0.45f),
        18.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightSlowAttackMs, 1),
        "EXP Tight Slow Attack (ms)",
        juce::NormalisableRange<float>(1.0f, 80.0f, 0.01f, 0.45f),
        12.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightSlowReleaseMs, 1),
        "EXP Tight Slow Release (ms)",
        juce::NormalisableRange<float>(20.0f, 600.0f, 0.1f, 0.45f),
        180.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightProgramAttackMs, 1),
        "EXP Tight Program Attack (ms)",
        juce::NormalisableRange<float>(10.0f, 600.0f, 0.1f, 0.45f),
        120.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightProgramReleaseMs, 1),
        "EXP Tight Program Release (ms)",
        juce::NormalisableRange<float>(100.0f, 4000.0f, 1.0f, 0.45f),
        1800.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightTransientSensitivity, 1),
        "EXP Tight Transient Sensitivity",
        juce::NormalisableRange<float>(0.25f, 6.0f, 0.01f, 0.45f),
        2.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightThresholdOffsetDb, 1),
        "EXP Tight Threshold Offset (dB)",
        juce::NormalisableRange<float>(-30.0f, 0.0f, 0.1f),
        -16.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightThresholdRangeDb, 1),
        "EXP Tight Threshold Range (dB)",
        juce::NormalisableRange<float>(3.0f, 36.0f, 0.1f, 0.45f),
        18.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightThresholdFloorDb, 1),
        "EXP Tight Threshold Floor (dB)",
        juce::NormalisableRange<float>(-96.0f, -12.0f, 0.1f),
        -72.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightThresholdCeilDb, 1),
        "EXP Tight Threshold Ceiling (dB)",
        juce::NormalisableRange<float>(-24.0f, 0.0f, 0.1f),
        -6.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expTightSustainCurve, 1),
        "EXP Tight Sustain Curve",
        juce::NormalisableRange<float>(0.2f, 3.0f, 0.01f, 0.45f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expOutputAutoMakeupDb, 1),
        "EXP Output Auto Makeup (dB)",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f),
        0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expOutputBodyHoldDb, 1),
        "EXP Output Body Hold (dB)",
        juce::NormalisableRange<float>(-6.0f, 6.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLimiterThresholdDb, 1),
        "EXP Limiter Threshold (dB)",
        juce::NormalisableRange<float>(-8.0f, 0.0f, 0.1f),
        -0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::expLimiterReleaseMs, 1),
        "EXP Limiter Release (ms)",
        juce::NormalisableRange<float>(10.0f, 400.0f, 0.1f, 0.45f),
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

void OsmiumAudioProcessor::parameterChanged(const juce::String& parameterID, float)
{
    if (parameterID != ParameterIDs::intensity && parameterID != ParameterIDs::expProcessingMode)
        return;

    macroIntensityPending.store(apvts.getRawParameterValue(ParameterIDs::intensity)->load());
    macroModePending.store(juce::jlimit(0, 2, juce::roundToInt(apvts.getRawParameterValue(ParameterIDs::expProcessingMode)->load())));
    macroSyncPending.store(true);
    triggerAsyncUpdate();
}

void OsmiumAudioProcessor::handleAsyncUpdate()
{
    if (!macroSyncPending.exchange(false))
        return;

    const auto mode = static_cast<ProcessingMode>(juce::jlimit(0, 2, macroModePending.load()));
    applyCoreMacroToExperimentalParameters(macroIntensityPending.load(), mode);
}

void OsmiumAudioProcessor::setParameterValueFromMacro(const char* id, float scaledValue)
{
    if (auto* rawValue = apvts.getRawParameterValue(id))
    {
        const float current = rawValue->load();
        if (std::abs(current - scaledValue) <= 1.0e-5f)
            return;
    }

    if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(id)))
    {
        const float normalised = param->convertTo0to1(scaledValue);
        param->setValueNotifyingHost(normalised);
    }
}

void OsmiumAudioProcessor::applyCoreMacroToExperimentalParameters(float intensity, ProcessingMode mode)
{
    const float i = juce::jlimit(0.0f, 1.0f, intensity);
    const float densityRegion = juce::jlimit(0.0f, 1.0f, (i - 0.60f) / 0.40f);
    const float density = densityRegion * densityRegion;
    const bool tightMode = (mode == ProcessingMode::tight);

    const float lowSatSkewed = std::pow(i, 0.6f);
    const float highSatSkewed = std::pow(i, 0.6f);

    float lowSatDrive = juce::jmap(lowSatSkewed, 1.0f, 1.94f);
    float highSatDrive = juce::jmap(highSatSkewed, 4.0f, 4.7392f) + density * 0.9408f;
    int lowWaveShaper = 1;  // Sine
    int highWaveShaper = 1; // Sine

    if (mode == ProcessingMode::chaotic)
    {
        lowWaveShaper = 2;  // Hard Clip
        highWaveShaper = 2; // Hard Clip
        lowSatDrive = juce::jmap(lowSatSkewed, 1.0f, 6.0f);
        highSatDrive = juce::jmap(highSatSkewed, 4.0f, 9.0f);
    }
    else if (tightMode)
    {
        lowWaveShaper = 2;  // Hard Clip
        highWaveShaper = 2; // Hard Clip
        lowSatDrive = 1.0f;
        highSatDrive = 2.92f;
    }

    const float lowSatMix = tightMode ? 0.0f : juce::jmap(lowSatSkewed, 0.0f, 1.0f);
    const float highSatMix = tightMode ? juce::jmap(highSatSkewed, 0.0f, 0.27f)
                                       : juce::jmap(highSatSkewed, 0.0f, 1.0f);
    const float highDriveDb = juce::jmap(i, 4.0f, 9.5f);
    const float highTrimSkewed = std::pow(i, 1.7f);
    const float highTrimDb = tightMode
        ? 0.0f
        : -(juce::jmap(highTrimSkewed, 0.1f, 0.8090909f) + density * 3.1909091f);

    const float lowAttackBoost = tightMode
        ? (juce::jmap(i, 0.0f, 2.5345454f) + density * 1.5654546f)
        : (juce::jmap(i, 0.0f, 7.3563636f) + density * 4.5436364f);
    const float lowPostAttackComp = tightMode
        ? (juce::jmap(i, 0.0f, 2.975f) + density * 3.825f)
        : (juce::jmap(i, 0.0f, 3.01875f) + density * 3.88125f);
    const float lowAttackTimeMs = tightMode ? 30.8f : juce::jmap(density, 14.0f, 30.8f);
    const float lowCompTimeMs = tightMode ? juce::jmap(density, 24.0f, 68.4f)
                                          : juce::jmap(density, 24.0f, 68.5f);
    const float lowBodyLiftDb = juce::jmap(i, 0.0f, 3.9666667f) + density * 7.9333333f;

    const float highAttackBoost = tightMode
        ? (juce::jmap(i, 0.0f, 8.6153846f) + density * 7.3846154f)
        : (juce::jmap(i, 0.0f, 8.5615385f) + density * 7.3384615f);
    const float highPostAttackComp = tightMode
        ? (juce::jmap(i, 0.0f, 2.3684211f) + density * 5.1315789f)
        : (juce::jmap(i, 0.0f, 2.3368421f) + density * 5.0631579f);
    const float highAttackTimeMs = juce::jmap(density, 6.0f, 12.7f);
    const float highCompTimeMs = juce::jmap(density, 14.0f, 38.1f);
    const float highTransientSensitivity = tightMode ? juce::jmap(i, 2.0f, 6.0f) : 2.0f;

    const float tightSustainDepth = tightMode ? juce::jmap(i, 0.0f, 6.5f) : 0.0f;
    const float autoMakeupDb = tightMode ? -3.8f : -(highSatMix * 2.4f + lowSatMix * 1.4f);
    const float bodyHoldDb = tightMode
        ? (juce::jmap(i, 0.0f, 0.68f) + density * 2.72f)
        : (juce::jmap(i, 0.0f, 0.2f) + density * 0.8f);

    setParameterValueFromMacro(ParameterIDs::expCrossoverHz, juce::jmap(i, 220.0f, 600.0f));

    setParameterValueFromMacro(ParameterIDs::expLowAttackBoostDb, lowAttackBoost);
    setParameterValueFromMacro(ParameterIDs::expLowPostCompDb, lowPostAttackComp);
    setParameterValueFromMacro(ParameterIDs::expLowAttackTimeMs, lowAttackTimeMs);
    setParameterValueFromMacro(ParameterIDs::expLowCompTimeMs, lowCompTimeMs);
    setParameterValueFromMacro(ParameterIDs::expLowBodyLiftDb, lowBodyLiftDb);
    setParameterValueFromMacro(ParameterIDs::expLowSatMix, lowSatMix);
    setParameterValueFromMacro(ParameterIDs::expLowSatDrive, lowSatDrive);
    setParameterValueFromMacro(ParameterIDs::expLowWaveShaperType, static_cast<float>(lowWaveShaper));
    setParameterValueFromMacro(ParameterIDs::expLowFastAttackMs, 1.0f);
    setParameterValueFromMacro(ParameterIDs::expLowFastReleaseMs, 15.0f);
    setParameterValueFromMacro(ParameterIDs::expLowSlowReleaseMs, 120.0f);
    setParameterValueFromMacro(ParameterIDs::expLowAttackGainSmoothMs, 1.8f);
    setParameterValueFromMacro(ParameterIDs::expLowCompAttackRatio, 0.25f);
    setParameterValueFromMacro(ParameterIDs::expLowCompGainSmoothMs, 1.6f);
    setParameterValueFromMacro(ParameterIDs::expLowTransientSensitivity, 2.0f);
    setParameterValueFromMacro(ParameterIDs::expLowLimiterThresholdDb, tightMode ? 0.0f : -0.15f);
    setParameterValueFromMacro(ParameterIDs::expLowLimiterReleaseMs, tightMode ? 55.0f : 180.0f);
    setParameterValueFromMacro(ParameterIDs::expBypassLowTransient, tightMode ? 1.0f : 0.0f);
    setParameterValueFromMacro(ParameterIDs::expBypassLowSaturation, tightMode ? 1.0f : 0.0f);
    setParameterValueFromMacro(ParameterIDs::expBypassLowLimiter, tightMode ? 1.0f : 0.0f);

    setParameterValueFromMacro(ParameterIDs::expHighAttackBoostDb, highAttackBoost);
    setParameterValueFromMacro(ParameterIDs::expHighPostCompDb, highPostAttackComp);
    setParameterValueFromMacro(ParameterIDs::expHighAttackTimeMs, highAttackTimeMs);
    setParameterValueFromMacro(ParameterIDs::expHighCompTimeMs, highCompTimeMs);
    setParameterValueFromMacro(ParameterIDs::expHighDriveDb, highDriveDb);
    setParameterValueFromMacro(ParameterIDs::expHighSatMix, highSatMix);
    setParameterValueFromMacro(ParameterIDs::expHighSatDrive, highSatDrive);
    setParameterValueFromMacro(ParameterIDs::expHighTrimDb, highTrimDb);
    setParameterValueFromMacro(ParameterIDs::expHighWaveShaperType, static_cast<float>(highWaveShaper));
    setParameterValueFromMacro(ParameterIDs::expHighFastAttackMs, tightMode ? 0.68f : 1.0f);
    setParameterValueFromMacro(ParameterIDs::expHighFastReleaseMs, tightMode ? 10.46f : 15.0f);
    setParameterValueFromMacro(ParameterIDs::expHighSlowReleaseMs, tightMode ? 20.0f : 120.0f);
    setParameterValueFromMacro(ParameterIDs::expHighAttackGainSmoothMs, tightMode ? 5.07f : 1.8f);
    setParameterValueFromMacro(ParameterIDs::expHighCompAttackRatio, tightMode ? 0.05f : 0.25f);
    setParameterValueFromMacro(ParameterIDs::expHighCompGainSmoothMs, tightMode ? 0.10f : 1.6f);
    setParameterValueFromMacro(ParameterIDs::expHighTransientSensitivity, highTransientSensitivity);

    setParameterValueFromMacro(ParameterIDs::expTightSustainDepthDb, tightSustainDepth);
    setParameterValueFromMacro(ParameterIDs::expTightSustainAttackMs, 30.0f);
    setParameterValueFromMacro(ParameterIDs::expTightReleaseMs, 46.0f);
    setParameterValueFromMacro(ParameterIDs::expTightBellFreqHz, 356.0f);
    setParameterValueFromMacro(ParameterIDs::expTightBellCutDb, 0.0f);
    setParameterValueFromMacro(ParameterIDs::expTightBellQ, 0.40f);
    setParameterValueFromMacro(ParameterIDs::expTightHighShelfCutDb, 0.0f);
    setParameterValueFromMacro(ParameterIDs::expTightHighShelfFreqHz, 7000.0f);
    setParameterValueFromMacro(ParameterIDs::expTightHighShelfQ, 0.71f);
    setParameterValueFromMacro(ParameterIDs::expTightFastAttackMs, 0.10f);
    setParameterValueFromMacro(ParameterIDs::expTightFastReleaseMs, 84.68f);
    setParameterValueFromMacro(ParameterIDs::expTightSlowAttackMs, 24.29f);
    setParameterValueFromMacro(ParameterIDs::expTightSlowReleaseMs, 600.0f);
    setParameterValueFromMacro(ParameterIDs::expTightProgramAttackMs, 80.40f);
    setParameterValueFromMacro(ParameterIDs::expTightProgramReleaseMs, 100.0f);
    setParameterValueFromMacro(ParameterIDs::expTightTransientSensitivity, 2.8f);
    setParameterValueFromMacro(ParameterIDs::expTightThresholdOffsetDb, -16.4f);
    setParameterValueFromMacro(ParameterIDs::expTightThresholdRangeDb, 17.9f);
    setParameterValueFromMacro(ParameterIDs::expTightThresholdFloorDb, -72.0f);
    setParameterValueFromMacro(ParameterIDs::expTightThresholdCeilDb, -11.1f);
    setParameterValueFromMacro(ParameterIDs::expTightSustainCurve, 1.7f);

    setParameterValueFromMacro(ParameterIDs::expOutputAutoMakeupDb, autoMakeupDb);
    setParameterValueFromMacro(ParameterIDs::expOutputBodyHoldDb, bodyHoldDb);
    setParameterValueFromMacro(ParameterIDs::expLimiterThresholdDb, -0.1f);
    setParameterValueFromMacro(ParameterIDs::expLimiterReleaseMs, 80.0f);
}

OsmiumAudioProcessor::MeterSnapshot OsmiumAudioProcessor::getMeterSnapshot() const
{
    return MeterSnapshot { meterInputDb.load(), meterOutputDb.load(), meterReductionDb.load() };
}

void OsmiumAudioProcessor::resetAllParametersToDefaults()
{
    for (auto* parameter : getParameters())
    {
        if (parameter == nullptr)
            continue;
        parameter->setValueNotifyingHost(parameter->getDefaultValue());
    }

    macroIntensityPending.store(apvts.getRawParameterValue(ParameterIDs::intensity)->load());
    macroModePending.store(juce::jlimit(0, 2, juce::roundToInt(apvts.getRawParameterValue(ParameterIDs::expProcessingMode)->load())));
    macroSyncPending.store(true);
    triggerAsyncUpdate();
}

void OsmiumAudioProcessor::smoothMeter(std::atomic<float>& meter, float targetDb, float decayDbPerBlock) const
{
    const float previous = meter.load();
    const float next = (targetDb > previous) ? targetDb : juce::jmax(targetDb, previous - decayDbPerBlock);
    meter.store(next);
}

float OsmiumAudioProcessor::getPeakMagnitude(const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax(peak, std::abs(data[i]));
    }

    return peak;
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

    dryLowpassFilter.prepare(spec);
    dryLowpassFilter.setCutoffFrequency(220.0f);
    dryLowpassFilter.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);

    dryHighpassFilter.prepare(spec);
    dryHighpassFilter.setCutoffFrequency(220.0f);
    dryHighpassFilter.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

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

    cleanLowPreShelfFilter.prepare(spec);
    cleanLowPostShelfFilter.prepare(spec);
    *cleanLowPreShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, 100.0f, 1.0f, juce::Decibels::decibelsToGain(-10.0f));
    *cleanLowPostShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, 100.0f, 1.0f, juce::Decibels::decibelsToGain(10.0f));

    dryBuffer.setSize(numChannels, samplesPerBlock);
    dryLowBandBuffer.setSize(numChannels, samplesPerBlock);
    dryHighBandBuffer.setSize(numChannels, samplesPerBlock);
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
    *tightHighShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, juce::jmin(7000.0f, static_cast<float>(sampleRate * 0.45f)), 0.7071f, 1.0f);

    tightFastEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
    tightSlowEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
    tightProgramEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
    tightGainDbEnvelope.assign(static_cast<size_t>(numChannels), 0.0f);
    tightLookaheadBufferLength = juce::jmax(1, juce::roundToInt(sampleRate * 0.001 * 2.0) + 1);
    tightLookaheadBuffers.assign(static_cast<size_t>(numChannels), std::vector<float>(static_cast<size_t>(tightLookaheadBufferLength), 0.0f));
    tightLookaheadWritePositions.assign(static_cast<size_t>(numChannels), 0);
    tightLookaheadSamplesCurrent = 0;

    dryDelayBufferLength = juce::jmax(1, juce::roundToInt(sampleRate * 0.05) + samplesPerBlock + 8);
    dryDelayBuffers.assign(static_cast<size_t>(numChannels), std::vector<float>(static_cast<size_t>(dryDelayBufferLength), 0.0f));
    dryDelayWritePositions.assign(static_cast<size_t>(numChannels), 0);
    dryDelaySamplesCurrent = 0;

    tightReportedLatencySamples = -1;
    setLatencySamples(0);

    smoothedIntensity.reset(sampleRate, 0.05);
    smoothedIntensity.setCurrentAndTargetValue(apvts.getRawParameterValue(ParameterIDs::intensity)->load());
    smoothedOutput.reset(sampleRate, 0.05);
    smoothedOutput.setCurrentAndTargetValue(apvts.getRawParameterValue(ParameterIDs::outputGain)->load());
    smoothedWetMix.reset(sampleRate, 0.02);
    smoothedWetMix.setCurrentAndTargetValue(juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue(ParameterIDs::intensity)->load() / 0.20f));
    smoothedFinalAgcDb.reset(sampleRate, 0.10);
    smoothedFinalAgcDb.setCurrentAndTargetValue(0.0f);
    lastProcessingMode = -1;
    lowBandTransientShaper.resetState();
    highBandTransientShaper.resetState();
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

    juce::ignoreUnused(preOff, postOff);
    processShaper(baseBlock);
}

void OsmiumAudioProcessor::applyTightnessStage(juce::AudioBuffer<float>& buffer,
                                               float sustainDepthDb,
                                               float sustainAttackMs,
                                               float sustainReleaseMs,
                                               float bellFreqHz,
                                               float bellCutDb,
                                               float bellQ,
                                               float highShelfCutDb,
                                               float highShelfFreqHz,
                                               float highShelfQ,
                                               float fastAttackMs,
                                               float fastReleaseMs,
                                               float slowAttackMs,
                                               float slowReleaseMs,
                                               float programAttackMs,
                                               float programReleaseMs,
                                               float transientSensitivity,
                                               float thresholdOffsetDb,
                                               float thresholdRangeDb,
                                               float thresholdFloorDb,
                                               float thresholdCeilDb,
                                               float sustainCurve,
                                               int lookaheadSamples)
{
    const auto sr = static_cast<float>(getSampleRate());
    if (sr <= 0.0f)
        return;

    sustainDepthDb = juce::jlimit(0.0f, 18.0f, sustainDepthDb);
    sustainAttackMs = juce::jlimit(0.2f, 30.0f, sustainAttackMs);
    sustainReleaseMs = juce::jlimit(20.0f, 220.0f, sustainReleaseMs);
    bellFreqHz = juce::jlimit(200.0f, 600.0f, bellFreqHz);
    bellCutDb = juce::jlimit(0.0f, 12.0f, bellCutDb);
    bellQ = juce::jlimit(0.4f, 1.8f, bellQ);
    highShelfCutDb = juce::jlimit(0.0f, 12.0f, highShelfCutDb);
    highShelfFreqHz = juce::jlimit(2500.0f, juce::jmin(12000.0f, sr * 0.45f), highShelfFreqHz);
    highShelfQ = juce::jlimit(0.3f, 2.0f, highShelfQ);
    fastAttackMs = juce::jlimit(0.1f, 10.0f, fastAttackMs);
    fastReleaseMs = juce::jlimit(1.0f, 120.0f, fastReleaseMs);
    slowAttackMs = juce::jlimit(1.0f, 80.0f, slowAttackMs);
    slowReleaseMs = juce::jlimit(20.0f, 600.0f, slowReleaseMs);
    programAttackMs = juce::jlimit(10.0f, 600.0f, programAttackMs);
    programReleaseMs = juce::jlimit(100.0f, 4000.0f, programReleaseMs);
    transientSensitivity = juce::jlimit(0.25f, 6.0f, transientSensitivity);
    thresholdOffsetDb = juce::jlimit(-30.0f, 0.0f, thresholdOffsetDb);
    thresholdRangeDb = juce::jlimit(3.0f, 36.0f, thresholdRangeDb);
    thresholdFloorDb = juce::jlimit(-96.0f, -12.0f, thresholdFloorDb);
    thresholdCeilDb = juce::jlimit(-24.0f, 0.0f, thresholdCeilDb);
    if (thresholdCeilDb < thresholdFloorDb + 1.0f)
        thresholdCeilDb = thresholdFloorDb + 1.0f;
    sustainCurve = juce::jlimit(0.2f, 3.0f, sustainCurve);
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
        sr, highShelfFreqHz, highShelfQ, juce::Decibels::decibelsToGain(-highShelfCutDb));

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

    const float fastAttackCoeff = std::exp(-1.0f / (sr * 0.001f * fastAttackMs));
    const float fastReleaseCoeff = std::exp(-1.0f / (sr * 0.001f * fastReleaseMs));
    const float slowAttackCoeff = std::exp(-1.0f / (sr * 0.001f * slowAttackMs));
    const float slowReleaseCoeff = std::exp(-1.0f / (sr * 0.001f * slowReleaseMs));
    const float programAttackCoeff = std::exp(-1.0f / (sr * 0.001f * programAttackMs));
    const float programReleaseCoeff = std::exp(-1.0f / (sr * 0.001f * programReleaseMs));
    const float sustainAttackCoeff = std::exp(-1.0f / (sr * 0.001f * sustainAttackMs));
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

            const float transientAmount = juce::jlimit(0.0f,
                                                       1.0f,
                                                       ((fastEnv - slowEnv) / (slowEnv + 1.0e-4f)) * transientSensitivity);
            const float sustainAmount = std::pow(juce::jlimit(0.0f, 1.0f, 1.0f - transientAmount), sustainCurve);
            const float levelDb = juce::Decibels::gainToDecibels(slowEnv + 1.0e-6f, -100.0f);
            const float programDb = juce::Decibels::gainToDecibels(programEnv + 1.0e-6f, -100.0f);
            const float autoThresholdDb = juce::jlimit(thresholdFloorDb, thresholdCeilDb, programDb + thresholdOffsetDb);
            const float thresholdGate = juce::jlimit(0.0f, 1.0f, (levelDb - autoThresholdDb) / thresholdRangeDb);
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

    const float inputPeak = getPeakMagnitude(buffer);
    const float inputDb = juce::Decibels::gainToDecibels(inputPeak + 1.0e-9f, -100.0f);

    if (*apvts.getRawParameterValue(ParameterIDs::bypass) > 0.5f)
    {
        smoothMeter(meterInputDb, inputDb, 1.5f);
        smoothMeter(meterOutputDb, inputDb, 1.5f);
        smoothMeter(meterReductionDb, 0.0f, 0.8f);
        return;
    }

    const auto getParam = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };

    dryBuffer.makeCopyOf(buffer, true);

    const float intensityTarget = getParam(ParameterIDs::intensity);
    const float outputTarget = getParam(ParameterIDs::outputGain);
    const bool cleanLowEndEnabled = getParam(ParameterIDs::cleanLowEnd) > 0.5f;
    const bool manualMode = getParam(ParameterIDs::expManualMode) > 0.5f;
    const auto processingMode = static_cast<ProcessingMode>(
        juce::jlimit(0, 2, juce::roundToInt(getParam(ParameterIDs::expProcessingMode))));
    const int modeIndex = static_cast<int>(processingMode);

    bool muteLowBand = getParam(ParameterIDs::expMuteLowBand) > 0.5f;
    bool muteHighBand = getParam(ParameterIDs::expMuteHighBand) > 0.5f;
    bool bypassLowTransient = getParam(ParameterIDs::expBypassLowTransient) > 0.5f;
    bool bypassLowSaturation = getParam(ParameterIDs::expBypassLowSaturation) > 0.5f;
    bool bypassLowLimiter = getParam(ParameterIDs::expBypassLowLimiter) > 0.5f;
    bool bypassHighTransient = getParam(ParameterIDs::expBypassHighTransient) > 0.5f;
    bool bypassHighSaturation = getParam(ParameterIDs::expBypassHighSaturation) > 0.5f;
    bool bypassTightness = getParam(ParameterIDs::expBypassTightness) > 0.5f;
    bool bypassOutputLimiter = getParam(ParameterIDs::expBypassOutputLimiter) > 0.5f;
    if (!manualMode)
    {
        muteLowBand = false;
        muteHighBand = false;
        bypassLowTransient = false;
        bypassLowSaturation = false;
        bypassLowLimiter = false;
        bypassHighTransient = false;
        bypassHighSaturation = false;
        bypassTightness = false;
        bypassOutputLimiter = false;
    }

    if (processingMode == ProcessingMode::tight)
    {
        bypassLowTransient = true;
        bypassLowSaturation = true;
        bypassLowLimiter = true;
    }

    const bool tightnessEnabled = (processingMode == ProcessingMode::tight) && !bypassTightness;

    if (modeIndex != lastProcessingMode)
    {
        lowBandTransientShaper.resetState();
        highBandTransientShaper.resetState();
        for (auto& v : tightFastEnvelope) v = 0.0f;
        for (auto& v : tightSlowEnvelope) v = 0.0f;
        for (auto& v : tightProgramEnvelope) v = 0.0f;
        for (auto& v : tightGainDbEnvelope) v = 0.0f;
        for (auto& channel : tightLookaheadBuffers) std::fill(channel.begin(), channel.end(), 0.0f);
        std::fill(tightLookaheadWritePositions.begin(), tightLookaheadWritePositions.end(), 0);
        tightLookaheadSamplesCurrent = 0;
        lastProcessingMode = modeIndex;
    }

    smoothedIntensity.setTargetValue(intensityTarget);
    smoothedOutput.setTargetValue(outputTarget);
    const float intensityStart = smoothedIntensity.getCurrentValue();
    const float outputStart = smoothedOutput.getCurrentValue();
    smoothedIntensity.skip(buffer.getNumSamples());
    smoothedOutput.skip(buffer.getNumSamples());
    const float intensityEnd = smoothedIntensity.getCurrentValue();
    const float outputEnd = smoothedOutput.getCurrentValue();

    smoothedWetMix.setTargetValue(juce::jlimit(0.0f, 1.0f, intensityTarget / 0.20f));
    const float wetMixStart = smoothedWetMix.getCurrentValue();
    smoothedWetMix.skip(buffer.getNumSamples());
    const float wetMixEnd = smoothedWetMix.getCurrentValue();

    const float finalAgcMaxDb = (processingMode == ProcessingMode::tight)
        ? -2.5f
        : (processingMode == ProcessingMode::chaotic ? -8.5f : -4.0f);
    smoothedFinalAgcDb.setTargetValue(
        juce::jmap(juce::jlimit(0.0f, 1.0f, intensityTarget), 0.0f, finalAgcMaxDb));
    const float agcStartDb = smoothedFinalAgcDb.getCurrentValue();
    smoothedFinalAgcDb.skip(buffer.getNumSamples());
    const float agcEndDb = smoothedFinalAgcDb.getCurrentValue();

    const float currentIntensity = 0.5f * (intensityStart + intensityEnd);
    const float currentOutput = 0.5f * (outputStart + outputEnd);
    const float intensity = juce::jlimit(0.0f, 1.0f, currentIntensity);
    const float densityRegion = juce::jlimit(0.0f, 1.0f, (intensity - 0.60f) / 0.40f);
    const float density = densityRegion * densityRegion;
    const float lowSatSkewed = std::pow(intensity, 0.6f);
    const float highSatSkewed = std::pow(intensity, 0.6f);

    float crossoverHz = getParam(ParameterIDs::expCrossoverHz);
    float lowAttackBoost = getParam(ParameterIDs::expLowAttackBoostDb);
    float lowPostAttackComp = getParam(ParameterIDs::expLowPostCompDb);
    float lowAttackTimeMs = getParam(ParameterIDs::expLowAttackTimeMs);
    float lowCompTimeMs = getParam(ParameterIDs::expLowCompTimeMs);
    float lowBodyLiftDb = getParam(ParameterIDs::expLowBodyLiftDb);
    float lowSatMix = getParam(ParameterIDs::expLowSatMix);
    float lowSatDrive = getParam(ParameterIDs::expLowSatDrive);
    float lowFastAttackMs = getParam(ParameterIDs::expLowFastAttackMs);
    float lowFastReleaseMs = getParam(ParameterIDs::expLowFastReleaseMs);
    float lowSlowReleaseMs = getParam(ParameterIDs::expLowSlowReleaseMs);
    float lowAttackGainSmoothMs = getParam(ParameterIDs::expLowAttackGainSmoothMs);
    float lowCompAttackRatio = getParam(ParameterIDs::expLowCompAttackRatio);
    float lowCompGainSmoothMs = getParam(ParameterIDs::expLowCompGainSmoothMs);
    float lowTransientSensitivity = getParam(ParameterIDs::expLowTransientSensitivity);
    float lowLimiterThresholdDb = getParam(ParameterIDs::expLowLimiterThresholdDb);
    float lowLimiterReleaseMs = getParam(ParameterIDs::expLowLimiterReleaseMs);

    float highAttackBoost = getParam(ParameterIDs::expHighAttackBoostDb);
    float highPostAttackComp = getParam(ParameterIDs::expHighPostCompDb);
    float highAttackTimeMs = getParam(ParameterIDs::expHighAttackTimeMs);
    float highCompTimeMs = getParam(ParameterIDs::expHighCompTimeMs);
    float highDriveDb = getParam(ParameterIDs::expHighDriveDb);
    float highSatMix = getParam(ParameterIDs::expHighSatMix);
    float highSatDrive = getParam(ParameterIDs::expHighSatDrive);
    float highBandTrimDb = getParam(ParameterIDs::expHighTrimDb);
    float highFastAttackMs = getParam(ParameterIDs::expHighFastAttackMs);
    float highFastReleaseMs = getParam(ParameterIDs::expHighFastReleaseMs);
    float highSlowReleaseMs = getParam(ParameterIDs::expHighSlowReleaseMs);
    float highAttackGainSmoothMs = getParam(ParameterIDs::expHighAttackGainSmoothMs);
    float highCompAttackRatio = getParam(ParameterIDs::expHighCompAttackRatio);
    float highCompGainSmoothMs = getParam(ParameterIDs::expHighCompGainSmoothMs);
    float highTransientSensitivity = getParam(ParameterIDs::expHighTransientSensitivity);

    float limiterThresholdDb = getParam(ParameterIDs::expLimiterThresholdDb);
    float limiterReleaseMs = getParam(ParameterIDs::expLimiterReleaseMs);
    float autoMakeupDb = getParam(ParameterIDs::expOutputAutoMakeupDb);
    float bodyHoldDb = getParam(ParameterIDs::expOutputBodyHoldDb);

    const int oversamplingMode = juce::jlimit(0, 2, juce::roundToInt(getParam(ParameterIDs::expOversamplingMode)));
    auto lowWaveShaperType = static_cast<WaveShaperType>(
        juce::jlimit(0, 2, juce::roundToInt(getParam(ParameterIDs::expLowWaveShaperType))));
    auto highWaveShaperType = static_cast<WaveShaperType>(
        juce::jlimit(0, 2, juce::roundToInt(getParam(ParameterIDs::expHighWaveShaperType))));

    float tightSustainDepthDb = getParam(ParameterIDs::expTightSustainDepthDb);
    float tightSustainAttackMs = getParam(ParameterIDs::expTightSustainAttackMs);
    float tightReleaseMs = getParam(ParameterIDs::expTightReleaseMs);
    float tightBellFreqHz = getParam(ParameterIDs::expTightBellFreqHz);
    float tightBellCutDb = getParam(ParameterIDs::expTightBellCutDb);
    float tightBellQ = getParam(ParameterIDs::expTightBellQ);
    float tightHighShelfCutDb = getParam(ParameterIDs::expTightHighShelfCutDb);
    float tightHighShelfFreqHz = getParam(ParameterIDs::expTightHighShelfFreqHz);
    float tightHighShelfQ = getParam(ParameterIDs::expTightHighShelfQ);
    float tightFastAttackMs = getParam(ParameterIDs::expTightFastAttackMs);
    float tightFastReleaseMs = getParam(ParameterIDs::expTightFastReleaseMs);
    float tightSlowAttackMs = getParam(ParameterIDs::expTightSlowAttackMs);
    float tightSlowReleaseMs = getParam(ParameterIDs::expTightSlowReleaseMs);
    float tightProgramAttackMs = getParam(ParameterIDs::expTightProgramAttackMs);
    float tightProgramReleaseMs = getParam(ParameterIDs::expTightProgramReleaseMs);
    float tightTransientSensitivity = getParam(ParameterIDs::expTightTransientSensitivity);
    float tightThresholdOffsetDb = getParam(ParameterIDs::expTightThresholdOffsetDb);
    float tightThresholdRangeDb = getParam(ParameterIDs::expTightThresholdRangeDb);
    float tightThresholdFloorDb = getParam(ParameterIDs::expTightThresholdFloorDb);
    float tightThresholdCeilDb = getParam(ParameterIDs::expTightThresholdCeilDb);
    float tightSustainCurve = getParam(ParameterIDs::expTightSustainCurve);

    if (!manualMode)
    {
        crossoverHz = juce::jmap(intensity, 220.0f, 600.0f);

        lowAttackBoost = juce::jmap(intensity, 0.0f, 7.3563636f) + density * 4.5436364f;
        lowPostAttackComp = juce::jmap(intensity, 0.0f, 3.01875f) + density * 3.88125f;
        lowAttackTimeMs = juce::jmap(density, 14.0f, 30.8f);
        lowCompTimeMs = juce::jmap(density, 24.0f, 68.5f);
        lowBodyLiftDb = juce::jmap(intensity, 0.0f, 3.9666667f) + density * 7.9333333f;
        lowSatMix = juce::jmap(lowSatSkewed, 0.0f, 1.0f);
        lowSatDrive = juce::jmap(lowSatSkewed, 1.0f, 1.94f);
        lowFastAttackMs = 1.0f;
        lowFastReleaseMs = 15.0f;
        lowSlowReleaseMs = 120.0f;
        lowAttackGainSmoothMs = 1.8f;
        lowCompAttackRatio = 0.25f;
        lowCompGainSmoothMs = 1.6f;
        lowTransientSensitivity = 2.0f;
        lowLimiterThresholdDb = -0.15f;
        lowLimiterReleaseMs = 180.0f;

        highAttackBoost = juce::jmap(intensity, 0.0f, 8.5615385f) + density * 7.3384615f;
        highPostAttackComp = juce::jmap(intensity, 0.0f, 2.3368421f) + density * 5.0631579f;
        highAttackTimeMs = juce::jmap(density, 6.0f, 12.7f);
        highCompTimeMs = juce::jmap(density, 14.0f, 38.1f);
        highDriveDb = juce::jmap(intensity, 4.0f, 9.5f);
        highSatMix = juce::jmap(highSatSkewed, 0.0f, 1.0f);
        highSatDrive = juce::jmap(highSatSkewed, 4.0f, 4.7392f) + density * 0.9408f;
        const float highTrimSkewed = std::pow(intensity, 1.7f);
        highBandTrimDb = -(juce::jmap(highTrimSkewed, 0.1f, 0.8090909f) + density * 3.1909091f);
        highFastAttackMs = 1.0f;
        highFastReleaseMs = 15.0f;
        highSlowReleaseMs = 120.0f;
        highAttackGainSmoothMs = 1.8f;
        highCompAttackRatio = 0.25f;
        highCompGainSmoothMs = 1.6f;
        highTransientSensitivity = 2.0f;

        lowWaveShaperType = WaveShaperType::sine;
        highWaveShaperType = WaveShaperType::sine;

        if (processingMode == ProcessingMode::tight)
        {
            lowAttackBoost = juce::jmap(intensity, 0.0f, 2.5345454f) + density * 1.5654546f;
            lowPostAttackComp = juce::jmap(intensity, 0.0f, 2.975f) + density * 3.825f;
            lowAttackTimeMs = 30.8f;
            lowCompTimeMs = juce::jmap(density, 24.0f, 68.4f);
            lowBodyLiftDb = juce::jmap(intensity, 0.0f, 3.9666667f) + density * 7.9333333f;
            lowSatMix = 0.0f;
            lowSatDrive = 1.0f;
            lowFastAttackMs = 1.0f;
            lowFastReleaseMs = 15.0f;
            lowSlowReleaseMs = 120.0f;
            lowAttackGainSmoothMs = 1.8f;
            lowCompAttackRatio = 0.25f;
            lowCompGainSmoothMs = 1.6f;
            lowTransientSensitivity = 2.0f;
            lowLimiterThresholdDb = 0.0f;
            lowLimiterReleaseMs = 55.0f;

            highAttackBoost = juce::jmap(intensity, 0.0f, 8.6153846f) + density * 7.3846154f;
            highPostAttackComp = juce::jmap(intensity, 0.0f, 2.3684211f) + density * 5.1315789f;
            highAttackTimeMs = juce::jmap(density, 6.0f, 12.7f);
            highCompTimeMs = juce::jmap(density, 14.0f, 38.1f);
            highDriveDb = juce::jmap(intensity, 4.0f, 9.5f);
            highSatMix = juce::jmap(highSatSkewed, 0.0f, 0.27f);
            highSatDrive = 2.92f;
            highBandTrimDb = 0.0f;
            highFastAttackMs = 0.68f;
            highFastReleaseMs = 10.46f;
            highSlowReleaseMs = 20.0f;
            highAttackGainSmoothMs = 5.07f;
            highCompAttackRatio = 0.05f;
            highCompGainSmoothMs = 0.10f;
            highTransientSensitivity = juce::jmap(intensity, 2.0f, 6.0f);

            lowWaveShaperType = WaveShaperType::hardClip;
            highWaveShaperType = WaveShaperType::hardClip;
        }
        else if (processingMode == ProcessingMode::chaotic)
        {
            lowWaveShaperType = WaveShaperType::hardClip;
            highWaveShaperType = WaveShaperType::hardClip;
            lowSatDrive = juce::jmap(lowSatSkewed, 1.0f, 6.0f);
            highSatDrive = juce::jmap(highSatSkewed, 4.0f, 9.0f);
        }

        tightSustainDepthDb = (processingMode == ProcessingMode::tight) ? juce::jmap(intensity, 0.0f, 6.5f) : 0.0f;
        tightSustainAttackMs = 30.0f;
        tightReleaseMs = 46.0f;
        tightBellFreqHz = 356.0f;
        tightBellCutDb = 0.0f;
        tightBellQ = 0.40f;
        tightHighShelfCutDb = 0.0f;
    // Phase-match dry path through the same crossover split/recombine.
    // ... we need to look at the exact end of the process block. 
    // Wait, the previous view_file was up to 1450, but the file has 1819 lines.
    // I need to view the end of processBlock first. I'll read from 1700-1819.
        tightHighShelfQ = 0.71f;
        tightFastAttackMs = 0.10f;
        tightFastReleaseMs = 84.68f;
        tightSlowAttackMs = 24.29f;
        tightSlowReleaseMs = 600.0f;
        tightProgramAttackMs = 80.40f;
        tightProgramReleaseMs = 100.0f;
        tightTransientSensitivity = 2.8f;
        tightThresholdOffsetDb = -16.4f;
        tightThresholdRangeDb = 17.9f;
        tightThresholdFloorDb = -72.0f;
        tightThresholdCeilDb = -11.1f;
        tightSustainCurve = 1.7f;

        autoMakeupDb = (processingMode == ProcessingMode::tight)
            ? -3.8f
            : -(highSatMix * 2.4f + lowSatMix * 1.4f);
        bodyHoldDb = (processingMode == ProcessingMode::tight)
            ? (juce::jmap(intensity, 0.0f, 0.68f) + density * 2.72f)
            : (juce::jmap(intensity, 0.0f, 0.2f) + density * 0.8f);
        limiterThresholdDb = -0.1f;
        limiterReleaseMs = 80.0f;
    }

    crossoverHz = juce::jlimit(220.0f, 600.0f, crossoverHz);
    lowSatMix = juce::jlimit(0.0f, 1.0f, lowSatMix);
    lowSatDrive = juce::jlimit(1.0f, 6.0f, lowSatDrive);
    highSatMix = juce::jlimit(0.0f, 1.0f, highSatMix);
    highSatDrive = juce::jlimit(1.0f, 9.0f, highSatDrive);
    highBandTrimDb = juce::jlimit(-4.0f, 0.0f, highBandTrimDb);
    tightSustainDepthDb = juce::jlimit(0.0f, 18.0f, tightSustainDepthDb);
    tightReleaseMs = juce::jlimit(20.0f, 220.0f, tightReleaseMs);
    tightBellFreqHz = juce::jlimit(200.0f, 600.0f, tightBellFreqHz);
    tightBellCutDb = juce::jlimit(0.0f, 12.0f, tightBellCutDb);
    tightBellQ = juce::jlimit(0.4f, 1.8f, tightBellQ);
    tightHighShelfCutDb = juce::jlimit(0.0f, 12.0f, tightHighShelfCutDb);
    tightHighShelfFreqHz = juce::jlimit(2500.0f, 12000.0f, tightHighShelfFreqHz);
    tightThresholdFloorDb = juce::jlimit(-96.0f, -12.0f, tightThresholdFloorDb);
    tightThresholdCeilDb = juce::jlimit(-24.0f, 0.0f, tightThresholdCeilDb);
    if (tightThresholdCeilDb < tightThresholdFloorDb + 1.0f)
        tightThresholdCeilDb = tightThresholdFloorDb + 1.0f;
    limiterThresholdDb = juce::jlimit(-8.0f, 0.0f, limiterThresholdDb);
    limiterReleaseMs = juce::jlimit(10.0f, 400.0f, limiterReleaseMs);
    lowLimiterThresholdDb = juce::jlimit(-12.0f, 0.0f, lowLimiterThresholdDb);
    lowLimiterReleaseMs = juce::jlimit(10.0f, 400.0f, lowLimiterReleaseMs);

    lowpassFilter.setCutoffFrequency(crossoverHz);
    highpassFilter.setCutoffFrequency(crossoverHz);
    dryLowpassFilter.setCutoffFrequency(crossoverHz);
    dryHighpassFilter.setCutoffFrequency(crossoverHz);
    lowBandLimiter.setThreshold(lowLimiterThresholdDb);
    lowBandLimiter.setRelease(lowLimiterReleaseMs);
    outputLimiter.setThreshold(limiterThresholdDb);
    outputLimiter.setRelease(limiterReleaseMs);

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (cleanLowEndEnabled)
    {
        juce::dsp::AudioBlock<float> preShelfBlock(buffer);
        juce::dsp::ProcessContextReplacing<float> preShelfContext(preShelfBlock);
        cleanLowPreShelfFilter.process(preShelfContext);
    }

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
    float reductionDb = 0.0f;

    auto updateReduction = [&reductionDb](float beforePeak, float afterPeak)
    {
        const float beforeDb = juce::Decibels::gainToDecibels(beforePeak + 1.0e-9f, -100.0f);
        const float afterDb = juce::Decibels::gainToDecibels(afterPeak + 1.0e-9f, -100.0f);
        reductionDb = juce::jmax(reductionDb, juce::jmax(0.0f, beforeDb - afterDb));
    };

    lowpassFilter.process(lowContext);
    highpassFilter.process(highContext);

    if (!bypassLowTransient)
    {
        lowBandTransientShaper.setParameters(lowAttackBoost,
                                             lowPostAttackComp,
                                             lowAttackTimeMs,
                                             lowCompTimeMs,
                                             lowFastAttackMs,
                                             lowFastReleaseMs,
                                             lowSlowReleaseMs,
                                             lowAttackGainSmoothMs,
                                             lowCompAttackRatio,
                                             lowCompGainSmoothMs,
                                             lowTransientSensitivity);
        lowBandTransientShaper.process(lowBandBuffer);
    }

    const float lowBodyLift = juce::Decibels::decibelsToGain(lowBodyLiftDb);
    if (std::abs(lowBodyLift - 1.0f) > 1.0e-5f)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            lowBandBuffer.applyGain(ch, 0, numSamples, lowBodyLift);
    }

    if (!bypassLowSaturation)
        applySaturationStage(lowBandBuffer, lowSatMix, lowSatDrive, oversamplingMode, lowWaveShaperType, SaturationBand::low);

    if (!bypassLowLimiter)
    {
        const float lowPreLimiterPeak = getPeakMagnitude(lowBandBuffer);
        lowBandLimiter.process(lowContext);
        const float lowPostLimiterPeak = getPeakMagnitude(lowBandBuffer);
        updateReduction(lowPreLimiterPeak, lowPostLimiterPeak);
    }

    if (!bypassHighTransient)
    {
        highBandTransientShaper.setParameters(highAttackBoost,
                                              highPostAttackComp,
                                              highAttackTimeMs,
                                              highCompTimeMs,
                                              highFastAttackMs,
                                              highFastReleaseMs,
                                              highSlowReleaseMs,
                                              highAttackGainSmoothMs,
                                              highCompAttackRatio,
                                              highCompGainSmoothMs,
                                              highTransientSensitivity);
        highBandTransientShaper.process(highBandBuffer);
    }

    juce::ignoreUnused(highDriveDb, highBandTrimDb);

    if (!bypassHighSaturation)
        applySaturationStage(highBandBuffer, highSatMix, highSatDrive, oversamplingMode, highWaveShaperType, SaturationBand::high);

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

    const int tightLookaheadSamples = tightnessEnabled
        ? juce::roundToInt(static_cast<float>(getSampleRate()) * 0.001f * 0.5f)
        : 0;

    if (tightnessEnabled)
    {
        const float preTightPeak = getPeakMagnitude(buffer);
        applyTightnessStage(buffer,
                            tightSustainDepthDb,
                            tightSustainAttackMs,
                            tightReleaseMs,
                            tightBellFreqHz,
                            tightBellCutDb,
                            tightBellQ,
                            tightHighShelfCutDb,
                            tightHighShelfFreqHz,
                            tightHighShelfQ,
                            tightFastAttackMs,
                            tightFastReleaseMs,
                            tightSlowAttackMs,
                            tightSlowReleaseMs,
                            tightProgramAttackMs,
                            tightProgramReleaseMs,
                            tightTransientSensitivity,
                            tightThresholdOffsetDb,
                            tightThresholdRangeDb,
                            tightThresholdFloorDb,
                            tightThresholdCeilDb,
                            tightSustainCurve,
                            tightLookaheadSamples);
        const float postTightPeak = getPeakMagnitude(buffer);
        updateReduction(preTightPeak, postTightPeak);
    }
    else if (manualMode)
    {
        for (auto& v : tightFastEnvelope) v = 0.0f;
        for (auto& v : tightSlowEnvelope) v = 0.0f;
        for (auto& v : tightProgramEnvelope) v = 0.0f;
        for (auto& v : tightGainDbEnvelope) v = 0.0f;
        for (auto& channel : tightLookaheadBuffers) std::fill(channel.begin(), channel.end(), 0.0f);
        std::fill(tightLookaheadWritePositions.begin(), tightLookaheadWritePositions.end(), 0);
        tightLookaheadSamplesCurrent = 0;
    }

    const int desiredLatencySamples = tightnessEnabled ? tightLookaheadSamples : 0;
    if (desiredLatencySamples != tightReportedLatencySamples)
    {
        setLatencySamples(desiredLatencySamples);
        tightReportedLatencySamples = desiredLatencySamples;
    }

    int oversamplingLatencySamples = 0;
    if (oversamplingMode == 1 && lowOversampling4x != nullptr)
        oversamplingLatencySamples = juce::jmax(0, juce::roundToInt(lowOversampling4x->getLatencyInSamples()));
    else if (oversamplingMode == 2 && lowOversampling8x != nullptr)
        oversamplingLatencySamples = juce::jmax(0, juce::roundToInt(lowOversampling8x->getLatencyInSamples()));

    if (dryDelayBufferLength <= 0)
        dryDelayBufferLength = juce::jmax(1, juce::roundToInt(getSampleRate() * 0.05) + numSamples + 8);

    const int desiredDryDelaySamples = juce::jlimit(0,
                                                    dryDelayBufferLength - 1,
                                                    desiredLatencySamples + oversamplingLatencySamples);
    dryDelaySamplesCurrent = desiredDryDelaySamples;

    const float chaoticPreClipDb = (processingMode == ProcessingMode::chaotic) ? juce::jmap(intensity, 1.0f, 12.0f) : 0.0f;
    const float preClipGainDb = autoMakeupDb + bodyHoldDb + chaoticPreClipDb;
    if (std::abs(preClipGainDb) > 1.0e-5f)
        buffer.applyGain(juce::Decibels::decibelsToGain(preClipGainDb));

    const float preOutputLimiterPeak = getPeakMagnitude(buffer);
    if (!bypassOutputLimiter)
    {
        const float clipperDrive = (processingMode == ProcessingMode::chaotic) ? juce::jmap(intensity, 2.2f, 3.8f) : 1.85f;
        applyOutputSoftClipper(buffer, -0.10f, clipperDrive);
    }
    const float postClipPeak = getPeakMagnitude(buffer);

    if (!bypassOutputLimiter)
        updateReduction(preOutputLimiterPeak, postClipPeak);

    if (cleanLowEndEnabled)
    {
        juce::dsp::AudioBlock<float> postShelfBlock(buffer);
        juce::dsp::ProcessContextReplacing<float> postShelfContext(postShelfBlock);
        cleanLowPostShelfFilter.process(postShelfContext);
    }

    // Phase-match dry path through the same crossover split/recombine.
    dryLowBandBuffer.setSize(numChannels, numSamples, false, false, true);
    dryHighBandBuffer.setSize(numChannels, numSamples, false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        dryLowBandBuffer.copyFrom(ch, 0, dryBuffer, ch, 0, numSamples);
        dryHighBandBuffer.copyFrom(ch, 0, dryBuffer, ch, 0, numSamples);
    }

    juce::dsp::AudioBlock<float> dryLowBlock(dryLowBandBuffer);
    juce::dsp::AudioBlock<float> dryHighBlock(dryHighBandBuffer);
    juce::dsp::ProcessContextReplacing<float> dryLowContext(dryLowBlock);
    juce::dsp::ProcessContextReplacing<float> dryHighContext(dryHighBlock);
    dryLowpassFilter.process(dryLowContext);
    dryHighpassFilter.process(dryHighContext);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* dryOut = dryBuffer.getWritePointer(ch);
        auto* dryLow = dryLowBandBuffer.getReadPointer(ch);
        auto* dryHigh = dryHighBandBuffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            dryOut[i] = dryLow[i] + dryHigh[i];
    }

    if ((int)dryDelayBuffers.size() != numChannels || (int)dryDelayWritePositions.size() != numChannels)
    {
        dryDelayBuffers.assign(static_cast<size_t>(numChannels), std::vector<float>(static_cast<size_t>(dryDelayBufferLength), 0.0f));
        dryDelayWritePositions.assign(static_cast<size_t>(numChannels), 0);
    }

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* dry = dryBuffer.getWritePointer(ch);
        auto& delayLine = dryDelayBuffers[static_cast<size_t>(ch)];
        int writePos = dryDelayWritePositions[static_cast<size_t>(ch)];

        for (int i = 0; i < numSamples; ++i)
        {
            delayLine[static_cast<size_t>(writePos)] = dry[i];

            int readPos = writePos - dryDelaySamplesCurrent;
            if (readPos < 0)
                readPos += dryDelayBufferLength;
            dry[i] = delayLine[static_cast<size_t>(readPos)];

            ++writePos;
            if (writePos >= dryDelayBufferLength)
                writePos = 0;
        }

        dryDelayWritePositions[static_cast<size_t>(ch)] = writePos;
    }

    const float wetMixStep = (numSamples > 1)
        ? ((wetMixEnd - wetMixStart) / static_cast<float>(numSamples - 1))
        : 0.0f;
    constexpr float halfPi = juce::MathConstants<float>::halfPi;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* wet = buffer.getWritePointer(ch);
        auto* dry = dryBuffer.getWritePointer(ch);
        float wetMix = wetMixStart;

        for (int i = 0; i < numSamples; ++i)
        {
            const float clampedMix = juce::jlimit(0.0f, 1.0f, wetMix);
            const float angle = clampedMix * halfPi;
            const float wetGain = std::sin(angle);
            const float dryGain = std::cos(angle);
            wet[i] = wet[i] * wetGain + dry[i] * dryGain;
            wetMix += wetMixStep;
        }
    }

    juce::dsp::AudioBlock<float> outputBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> outputContext(outputBlock);
    outputGain.setGainDecibels(currentOutput);
    outputGain.process(outputContext);

    if (std::abs(agcStartDb) > 1.0e-5f || std::abs(agcEndDb) > 1.0e-5f)
    {
        const float agcStartGain = juce::Decibels::decibelsToGain(agcStartDb);
        const float agcEndGain = juce::Decibels::decibelsToGain(agcEndDb);
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.applyGainRamp(ch, 0, numSamples, agcStartGain, agcEndGain);
    }

    const float outputPeak = getPeakMagnitude(buffer);
    const float outputDb = juce::Decibels::gainToDecibels(outputPeak + 1.0e-9f, -100.0f);
    smoothMeter(meterInputDb, inputDb, 1.5f);
    smoothMeter(meterOutputDb, outputDb, 1.5f);
    smoothMeter(meterReductionDb, reductionDb, 0.8f);
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
