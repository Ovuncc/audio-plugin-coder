#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "ParameterIDs.hpp"
#include "PluginProcessor.h"

class NeuralPanicAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit NeuralPanicAudioProcessorEditor(NeuralPanicAudioProcessor&);
    ~NeuralPanicAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    NeuralPanicAudioProcessor& audioProcessor_;

    // 1) Relays first.
    juce::WebSliderRelay modeRelay_ { ParameterIDs::mode };
    juce::WebSliderRelay panicRelay_ { ParameterIDs::panic };
    juce::WebSliderRelay bandwidthRelay_ { ParameterIDs::bandwidth };
    juce::WebSliderRelay collapseRelay_ { ParameterIDs::collapse };
    juce::WebSliderRelay jitterRelay_ { ParameterIDs::jitter };
    juce::WebSliderRelay pitchDriftRelay_ { ParameterIDs::pitchDrift };
    juce::WebSliderRelay formantMeltRelay_ { ParameterIDs::formantMelt };
    juce::WebSliderRelay broadcastRelay_ { ParameterIDs::broadcast };
    juce::WebSliderRelay smearRelay_ { ParameterIDs::smear };
    juce::WebSliderRelay seedRelay_ { ParameterIDs::seed };
    juce::WebSliderRelay mixRelay_ { ParameterIDs::mix };
    juce::WebSliderRelay outputGainRelay_ { ParameterIDs::outputGain };
    juce::WebToggleButtonRelay bypassRelay_ { ParameterIDs::bypass };
    juce::WebToggleButtonRelay codecBypassRelay_ { ParameterIDs::codecBypass };
    juce::WebToggleButtonRelay packetBypassRelay_ { ParameterIDs::packetBypass };
    juce::WebToggleButtonRelay identityBypassRelay_ { ParameterIDs::identityBypass };
    juce::WebToggleButtonRelay dynamicsBypassRelay_ { ParameterIDs::dynamicsBypass };
    juce::WebToggleButtonRelay temporalBypassRelay_ { ParameterIDs::temporalBypass };

    // 2) WebView second.
    std::unique_ptr<juce::WebBrowserComponent> webView_;

    // 3) Attachments last.
    std::unique_ptr<juce::WebSliderParameterAttachment> modeAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> panicAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> bandwidthAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> collapseAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> jitterAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchDriftAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> formantMeltAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> broadcastAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> smearAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> seedAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment_;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> bypassAttachment_;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> codecBypassAttachment_;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> packetBypassAttachment_;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> identityBypassAttachment_;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> dynamicsBypassAttachment_;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> temporalBypassAttachment_;

    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeuralPanicAudioProcessorEditor)
};
