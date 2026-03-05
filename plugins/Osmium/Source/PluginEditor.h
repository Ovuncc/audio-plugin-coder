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

    juce::WebSliderRelay intensityRelay { ParameterIDs::intensity };
    juce::WebSliderRelay outputGainRelay { ParameterIDs::outputGain };
    juce::WebToggleButtonRelay bypassRelay { ParameterIDs::bypass };
    juce::WebToggleButtonRelay cleanLowEndRelay { ParameterIDs::cleanLowEnd };
    juce::WebComboBoxRelay oversamplingRelay { ParameterIDs::oversamplingMode };
    juce::WebComboBoxRelay processingModeRelay { ParameterIDs::processingMode };
    juce::WebComboBoxRelay tightLookaheadRelay { ParameterIDs::tightLookaheadMode };

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>> comboAttachments;

    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OsmiumAudioProcessorEditor)
};
