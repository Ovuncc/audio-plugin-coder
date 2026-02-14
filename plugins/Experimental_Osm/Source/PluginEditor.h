#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

class OsmiumAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    OsmiumAudioProcessorEditor (OsmiumAudioProcessor&);
    ~OsmiumAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OsmiumAudioProcessor& audioProcessor;

    // Relays
    juce::WebSliderRelay intensityRelay { ParameterIDs::intensity };
    juce::WebSliderRelay outputGainRelay { ParameterIDs::outputGain };
    juce::WebToggleButtonRelay bypassRelay { ParameterIDs::bypass };
    juce::WebToggleButtonRelay expManualModeRelay { ParameterIDs::expManualMode };
    juce::WebToggleButtonRelay expMuteLowBandRelay { ParameterIDs::expMuteLowBand };
    juce::WebToggleButtonRelay expMuteHighBandRelay { ParameterIDs::expMuteHighBand };

    juce::WebSliderRelay expCrossoverRelay { ParameterIDs::expCrossoverHz };

    juce::WebSliderRelay expLowAttackBoostRelay { ParameterIDs::expLowAttackBoostDb };
    juce::WebSliderRelay expLowAttackTimeRelay { ParameterIDs::expLowAttackTimeMs };
    juce::WebSliderRelay expLowPostCompRelay { ParameterIDs::expLowPostCompDb };
    juce::WebSliderRelay expLowCompTimeRelay { ParameterIDs::expLowCompTimeMs };
    juce::WebSliderRelay expLowBodyLiftRelay { ParameterIDs::expLowBodyLiftDb };
    juce::WebSliderRelay expLowSatMixRelay { ParameterIDs::expLowSatMix };
    juce::WebSliderRelay expLowSatDriveRelay { ParameterIDs::expLowSatDrive };

    juce::WebSliderRelay expHighAttackBoostRelay { ParameterIDs::expHighAttackBoostDb };
    juce::WebSliderRelay expHighAttackTimeRelay { ParameterIDs::expHighAttackTimeMs };
    juce::WebSliderRelay expHighPostCompRelay { ParameterIDs::expHighPostCompDb };
    juce::WebSliderRelay expHighCompTimeRelay { ParameterIDs::expHighCompTimeMs };
    juce::WebSliderRelay expHighDriveRelay { ParameterIDs::expHighDriveDb };
    juce::WebSliderRelay expHighSatMixRelay { ParameterIDs::expHighSatMix };
    juce::WebSliderRelay expHighSatDriveRelay { ParameterIDs::expHighSatDrive };
    juce::WebSliderRelay expHighTrimRelay { ParameterIDs::expHighTrimDb };

    juce::WebSliderRelay expLimiterThresholdRelay { ParameterIDs::expLimiterThresholdDb };
    juce::WebSliderRelay expLimiterReleaseRelay { ParameterIDs::expLimiterReleaseMs };

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;

    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OsmiumAudioProcessorEditor)
};
