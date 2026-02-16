#pragma once

#include <memory>
#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "ParameterIDs.hpp"
#include "PluginProcessor.h"

class MorphantAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MorphantAudioProcessorEditor(MorphantAudioProcessor&);
    ~MorphantAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MorphantAudioProcessor& audioProcessor;

    // Order is intentional: relays -> webview -> attachments.
    juce::WebSliderRelay mergeRelay { ParameterIDs::merge };
    juce::WebSliderRelay glueRelay { ParameterIDs::glue };
    juce::WebSliderRelay pitchFollowerRelay { ParameterIDs::pitchFollower };
    juce::WebSliderRelay formantFollowerRelay { ParameterIDs::formantFollower };
    juce::WebSliderRelay focusRelay { ParameterIDs::focus };
    juce::WebSliderRelay realityRelay { ParameterIDs::reality };
    juce::WebSliderRelay sibilanceRelay { ParameterIDs::sibilance };
    juce::WebSliderRelay mixRelay { ParameterIDs::mix };
    juce::WebSliderRelay outputGainRelay { ParameterIDs::outputGain };
    juce::WebComboBoxRelay modeRelay { ParameterIDs::mode };

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebSliderParameterAttachment> mergeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> glueAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchFollowerAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> formantFollowerAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> focusAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> realityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sibilanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> modeAttachment;

    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MorphantAudioProcessorEditor)
};
