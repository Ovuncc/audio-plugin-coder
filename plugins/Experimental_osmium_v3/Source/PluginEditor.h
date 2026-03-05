#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

class OsmiumAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    OsmiumAudioProcessorEditor (OsmiumAudioProcessor&);
    ~OsmiumAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;

private:
    void timerCallback() override;

    OsmiumAudioProcessor& audioProcessor;

    // Relays
    juce::WebSliderRelay intensityRelay { ParameterIDs::intensity };
    juce::WebSliderRelay outputGainRelay { ParameterIDs::outputGain };
    juce::WebToggleButtonRelay bypassRelay { ParameterIDs::bypass };
    juce::WebToggleButtonRelay cleanLowEndRelay { ParameterIDs::cleanLowEnd };
    juce::WebToggleButtonRelay expManualModeRelay { ParameterIDs::expManualMode };
    juce::WebToggleButtonRelay expMuteLowBandRelay { ParameterIDs::expMuteLowBand };
    juce::WebToggleButtonRelay expMuteHighBandRelay { ParameterIDs::expMuteHighBand };

    juce::WebSliderRelay expCrossoverRelay { ParameterIDs::expCrossoverHz };
    juce::WebComboBoxRelay expOversamplingRelay { ParameterIDs::expOversamplingMode };
    juce::WebComboBoxRelay expProcessingModeRelay { ParameterIDs::expProcessingMode };
    juce::WebToggleButtonRelay expBypassLowTransientRelay { ParameterIDs::expBypassLowTransient };
    juce::WebToggleButtonRelay expBypassLowSaturationRelay { ParameterIDs::expBypassLowSaturation };
    juce::WebToggleButtonRelay expBypassLowLimiterRelay { ParameterIDs::expBypassLowLimiter };
    juce::WebToggleButtonRelay expBypassHighTransientRelay { ParameterIDs::expBypassHighTransient };
    juce::WebToggleButtonRelay expBypassHighSaturationRelay { ParameterIDs::expBypassHighSaturation };
    juce::WebToggleButtonRelay expBypassTightnessRelay { ParameterIDs::expBypassTightness };
    juce::WebToggleButtonRelay expBypassOutputLimiterRelay { ParameterIDs::expBypassOutputLimiter };

    juce::WebSliderRelay expLowAttackBoostRelay { ParameterIDs::expLowAttackBoostDb };
    juce::WebSliderRelay expLowAttackTimeRelay { ParameterIDs::expLowAttackTimeMs };
    juce::WebSliderRelay expLowPostCompRelay { ParameterIDs::expLowPostCompDb };
    juce::WebSliderRelay expLowCompTimeRelay { ParameterIDs::expLowCompTimeMs };
    juce::WebSliderRelay expLowBodyLiftRelay { ParameterIDs::expLowBodyLiftDb };
    juce::WebSliderRelay expLowSatMixRelay { ParameterIDs::expLowSatMix };
    juce::WebSliderRelay expLowSatDriveRelay { ParameterIDs::expLowSatDrive };
    juce::WebComboBoxRelay expLowWaveShaperRelay { ParameterIDs::expLowWaveShaperType };
    juce::WebSliderRelay expLowFastAttackRelay { ParameterIDs::expLowFastAttackMs };
    juce::WebSliderRelay expLowFastReleaseRelay { ParameterIDs::expLowFastReleaseMs };
    juce::WebSliderRelay expLowSlowReleaseRelay { ParameterIDs::expLowSlowReleaseMs };
    juce::WebSliderRelay expLowAttackGainSmoothRelay { ParameterIDs::expLowAttackGainSmoothMs };
    juce::WebSliderRelay expLowCompAttackRatioRelay { ParameterIDs::expLowCompAttackRatio };
    juce::WebSliderRelay expLowCompGainSmoothRelay { ParameterIDs::expLowCompGainSmoothMs };
    juce::WebSliderRelay expLowTransientSensitivityRelay { ParameterIDs::expLowTransientSensitivity };
    juce::WebSliderRelay expLowLimiterThresholdRelay { ParameterIDs::expLowLimiterThresholdDb };
    juce::WebSliderRelay expLowLimiterReleaseRelay { ParameterIDs::expLowLimiterReleaseMs };

    juce::WebSliderRelay expHighAttackBoostRelay { ParameterIDs::expHighAttackBoostDb };
    juce::WebSliderRelay expHighAttackTimeRelay { ParameterIDs::expHighAttackTimeMs };
    juce::WebSliderRelay expHighPostCompRelay { ParameterIDs::expHighPostCompDb };
    juce::WebSliderRelay expHighCompTimeRelay { ParameterIDs::expHighCompTimeMs };
    juce::WebSliderRelay expHighDriveRelay { ParameterIDs::expHighDriveDb };
    juce::WebSliderRelay expHighSatMixRelay { ParameterIDs::expHighSatMix };
    juce::WebSliderRelay expHighSatDriveRelay { ParameterIDs::expHighSatDrive };
    juce::WebSliderRelay expHighTrimRelay { ParameterIDs::expHighTrimDb };
    juce::WebComboBoxRelay expHighWaveShaperRelay { ParameterIDs::expHighWaveShaperType };
    juce::WebSliderRelay expHighFastAttackRelay { ParameterIDs::expHighFastAttackMs };
    juce::WebSliderRelay expHighFastReleaseRelay { ParameterIDs::expHighFastReleaseMs };
    juce::WebSliderRelay expHighSlowReleaseRelay { ParameterIDs::expHighSlowReleaseMs };
    juce::WebSliderRelay expHighAttackGainSmoothRelay { ParameterIDs::expHighAttackGainSmoothMs };
    juce::WebSliderRelay expHighCompAttackRatioRelay { ParameterIDs::expHighCompAttackRatio };
    juce::WebSliderRelay expHighCompGainSmoothRelay { ParameterIDs::expHighCompGainSmoothMs };
    juce::WebSliderRelay expHighTransientSensitivityRelay { ParameterIDs::expHighTransientSensitivity };

    juce::WebSliderRelay expTightSustainDepthRelay { ParameterIDs::expTightSustainDepthDb };
    juce::WebSliderRelay expTightSustainAttackRelay { ParameterIDs::expTightSustainAttackMs };
    juce::WebSliderRelay expTightReleaseRelay { ParameterIDs::expTightReleaseMs };
    juce::WebSliderRelay expTightBellFreqRelay { ParameterIDs::expTightBellFreqHz };
    juce::WebSliderRelay expTightBellCutRelay { ParameterIDs::expTightBellCutDb };
    juce::WebSliderRelay expTightBellQRelay { ParameterIDs::expTightBellQ };
    juce::WebSliderRelay expTightHighShelfCutRelay { ParameterIDs::expTightHighShelfCutDb };
    juce::WebSliderRelay expTightHighShelfFreqRelay { ParameterIDs::expTightHighShelfFreqHz };
    juce::WebSliderRelay expTightHighShelfQRelay { ParameterIDs::expTightHighShelfQ };
    juce::WebSliderRelay expTightFastAttackRelay { ParameterIDs::expTightFastAttackMs };
    juce::WebSliderRelay expTightFastReleaseRelay { ParameterIDs::expTightFastReleaseMs };
    juce::WebSliderRelay expTightSlowAttackRelay { ParameterIDs::expTightSlowAttackMs };
    juce::WebSliderRelay expTightSlowReleaseRelay { ParameterIDs::expTightSlowReleaseMs };
    juce::WebSliderRelay expTightProgramAttackRelay { ParameterIDs::expTightProgramAttackMs };
    juce::WebSliderRelay expTightProgramReleaseRelay { ParameterIDs::expTightProgramReleaseMs };
    juce::WebSliderRelay expTightTransientSensitivityRelay { ParameterIDs::expTightTransientSensitivity };
    juce::WebSliderRelay expTightThresholdOffsetRelay { ParameterIDs::expTightThresholdOffsetDb };
    juce::WebSliderRelay expTightThresholdRangeRelay { ParameterIDs::expTightThresholdRangeDb };
    juce::WebSliderRelay expTightThresholdFloorRelay { ParameterIDs::expTightThresholdFloorDb };
    juce::WebSliderRelay expTightThresholdCeilRelay { ParameterIDs::expTightThresholdCeilDb };
    juce::WebSliderRelay expTightSustainCurveRelay { ParameterIDs::expTightSustainCurve };

    juce::WebSliderRelay expOutputAutoMakeupRelay { ParameterIDs::expOutputAutoMakeupDb };
    juce::WebSliderRelay expOutputBodyHoldRelay { ParameterIDs::expOutputBodyHoldDb };

    juce::WebSliderRelay expLimiterThresholdRelay { ParameterIDs::expLimiterThresholdDb };
    juce::WebSliderRelay expLimiterReleaseRelay { ParameterIDs::expLimiterReleaseMs };

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>> comboAttachments;

    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OsmiumAudioProcessorEditor)
};
