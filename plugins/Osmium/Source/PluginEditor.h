#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

//==============================================================================
/**
*/
class OsmiumAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    OsmiumAudioProcessorEditor (OsmiumAudioProcessor&);
    ~OsmiumAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OsmiumAudioProcessor& audioProcessor;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: Member Declaration Order (Prevents DAW Crashes)
    // ═══════════════════════════════════════════════════════════════════

    // 1. PARAMETER RELAYS FIRST (no dependencies, destroyed last)
    juce::WebSliderRelay intensityRelay { ParameterIDs::intensity };
    juce::WebSliderRelay outputGainRelay { ParameterIDs::outputGain };
    juce::WebToggleButtonRelay bypassRelay { ParameterIDs::bypass };

    // 2. WEBVIEW SECOND (depends on relays, destroyed middle)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. PARAMETER ATTACHMENTS LAST (depend on relays + parameters, destroyed first)
    std::unique_ptr<juce::WebSliderParameterAttachment> intensityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> bypassAttachment;

    // Resource provider function
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OsmiumAudioProcessorEditor)
};
