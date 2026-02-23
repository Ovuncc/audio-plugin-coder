#include "PluginEditor.h"

#include <BinaryData.h>
#include <cstring>

NeuralPanicAudioProcessorEditor::NeuralPanicAudioProcessorEditor(NeuralPanicAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor_(p)
{
    auto userDataFolder = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory)
        .getChildFile("NeuralPanic_WebView2_Data_" + juce::String(juce::Time::getMillisecondCounter()));

    webView_ = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(userDataFolder))
            .withNativeIntegrationEnabled()
            .withOptionsFrom(modeRelay_)
            .withOptionsFrom(panicRelay_)
            .withOptionsFrom(bandwidthRelay_)
            .withOptionsFrom(collapseRelay_)
            .withOptionsFrom(jitterRelay_)
            .withOptionsFrom(pitchDriftRelay_)
            .withOptionsFrom(formantMeltRelay_)
            .withOptionsFrom(broadcastRelay_)
            .withOptionsFrom(smearRelay_)
            .withOptionsFrom(seedRelay_)
            .withOptionsFrom(mixRelay_)
            .withOptionsFrom(outputGainRelay_)
            .withOptionsFrom(bypassRelay_)
            .withOptionsFrom(codecBypassRelay_)
            .withOptionsFrom(packetBypassRelay_)
            .withOptionsFrom(identityBypassRelay_)
            .withOptionsFrom(dynamicsBypassRelay_)
            .withOptionsFrom(temporalBypassRelay_)
            .withResourceProvider([this](const auto& url) { return getResource(url); }));

    addAndMakeVisible(*webView_);

    auto& apvts = audioProcessor_.getAPVTS();

    auto attachSlider = [&](const char* id, juce::WebSliderRelay& relay,
                            std::unique_ptr<juce::WebSliderParameterAttachment>& attachment)
    {
        if (auto* parameter = apvts.getParameter(id))
            attachment = std::make_unique<juce::WebSliderParameterAttachment>(*parameter, relay, nullptr);
        else
            jassertfalse;
    };

    attachSlider(ParameterIDs::mode, modeRelay_, modeAttachment_);
    attachSlider(ParameterIDs::panic, panicRelay_, panicAttachment_);
    attachSlider(ParameterIDs::bandwidth, bandwidthRelay_, bandwidthAttachment_);
    attachSlider(ParameterIDs::collapse, collapseRelay_, collapseAttachment_);
    attachSlider(ParameterIDs::jitter, jitterRelay_, jitterAttachment_);
    attachSlider(ParameterIDs::pitchDrift, pitchDriftRelay_, pitchDriftAttachment_);
    attachSlider(ParameterIDs::formantMelt, formantMeltRelay_, formantMeltAttachment_);
    attachSlider(ParameterIDs::broadcast, broadcastRelay_, broadcastAttachment_);
    attachSlider(ParameterIDs::smear, smearRelay_, smearAttachment_);
    attachSlider(ParameterIDs::seed, seedRelay_, seedAttachment_);
    attachSlider(ParameterIDs::mix, mixRelay_, mixAttachment_);
    attachSlider(ParameterIDs::outputGain, outputGainRelay_, outputGainAttachment_);

    if (auto* parameter = apvts.getParameter(ParameterIDs::bypass))
        bypassAttachment_ = std::make_unique<juce::WebToggleButtonParameterAttachment>(*parameter, bypassRelay_, nullptr);
    else
        jassertfalse;

    if (auto* parameter = apvts.getParameter(ParameterIDs::codecBypass))
        codecBypassAttachment_ = std::make_unique<juce::WebToggleButtonParameterAttachment>(*parameter, codecBypassRelay_, nullptr);
    else
        jassertfalse;

    if (auto* parameter = apvts.getParameter(ParameterIDs::packetBypass))
        packetBypassAttachment_ = std::make_unique<juce::WebToggleButtonParameterAttachment>(*parameter, packetBypassRelay_, nullptr);
    else
        jassertfalse;

    if (auto* parameter = apvts.getParameter(ParameterIDs::identityBypass))
        identityBypassAttachment_ = std::make_unique<juce::WebToggleButtonParameterAttachment>(*parameter, identityBypassRelay_, nullptr);
    else
        jassertfalse;

    if (auto* parameter = apvts.getParameter(ParameterIDs::dynamicsBypass))
        dynamicsBypassAttachment_ = std::make_unique<juce::WebToggleButtonParameterAttachment>(*parameter, dynamicsBypassRelay_, nullptr);
    else
        jassertfalse;

    if (auto* parameter = apvts.getParameter(ParameterIDs::temporalBypass))
        temporalBypassAttachment_ = std::make_unique<juce::WebToggleButtonParameterAttachment>(*parameter, temporalBypassRelay_, nullptr);
    else
        jassertfalse;

    webView_->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    setSize(980, 620);
}

NeuralPanicAudioProcessorEditor::~NeuralPanicAudioProcessorEditor() = default;

void NeuralPanicAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void NeuralPanicAudioProcessorEditor::resized()
{
    if (webView_ != nullptr)
        webView_->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource> NeuralPanicAudioProcessorEditor::getResource(const juce::String& url)
{
    const auto path = (url == "/")
        ? juce::String("index.html")
        : url.fromFirstOccurrenceOf("/", false, false);

    const char* data = nullptr;
    int size = 0;
    const char* mime = "text/plain";

    if (path == "index.html")
    {
        data = BinaryData::index_html;
        size = BinaryData::index_htmlSize;
        mime = "text/html";
    }
    else if (path == "js/app.js")
    {
        data = BinaryData::app_js;
        size = BinaryData::app_jsSize;
        mime = "text/javascript";
    }
    else if (path == "js/juce/index.js")
    {
        data = BinaryData::index_js;
        size = BinaryData::index_jsSize;
        mime = "text/javascript";
    }
    else if (path == "js/juce/check_native_interop.js")
    {
        data = BinaryData::check_native_interop_js;
        size = BinaryData::check_native_interop_jsSize;
        mime = "text/javascript";
    }

    if (data != nullptr && size > 0)
    {
        std::vector<std::byte> bytes(static_cast<size_t>(size));
        std::memcpy(bytes.data(), data, static_cast<size_t>(size));
        return juce::WebBrowserComponent::Resource{ std::move(bytes), juce::String(mime) };
    }

    return std::nullopt;
}
