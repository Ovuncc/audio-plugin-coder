#include "PluginEditor.h"

#include <BinaryData.h>
#include <cstring>

MorphantAudioProcessorEditor::MorphantAudioProcessorEditor(MorphantAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , audioProcessor(p)
{
    auto options = juce::WebBrowserComponent::Options()
        .withNativeIntegrationEnabled()
        .withOptionsFrom(mergeRelay)
        .withOptionsFrom(glueRelay)
        .withOptionsFrom(pitchFollowerRelay)
        .withOptionsFrom(formantFollowerRelay)
        .withOptionsFrom(focusRelay)
        .withOptionsFrom(realityRelay)
        .withOptionsFrom(sibilanceRelay)
        .withOptionsFrom(mixRelay)
        .withOptionsFrom(outputGainRelay)
        .withOptionsFrom(modeRelay)
        .withOptionsFrom(debugModeRelay)
        .withResourceProvider([this](const auto& url) { return getResource(url); });

   #if JUCE_WINDOWS
    const auto userDataFolder = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory)
                                    .getChildFile("Morphant_WebView2_" + juce::String(juce::Time::getMillisecondCounter()));

    options = options.withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
                     .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}
                                                 .withUserDataFolder(userDataFolder));
   #endif

    webView = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*webView);

    auto attachSlider = [this](const char* paramId,
                               juce::WebSliderRelay& relay,
                               std::unique_ptr<juce::WebSliderParameterAttachment>& attachment)
    {
        if (auto* param = audioProcessor.getAPVTS().getParameter(paramId); param != nullptr)
        {
            attachment = std::make_unique<juce::WebSliderParameterAttachment>(*param, relay, nullptr);
        }
        else
        {
            jassertfalse;
        }
    };

    attachSlider(ParameterIDs::merge, mergeRelay, mergeAttachment);
    attachSlider(ParameterIDs::glue, glueRelay, glueAttachment);
    attachSlider(ParameterIDs::pitchFollower, pitchFollowerRelay, pitchFollowerAttachment);
    attachSlider(ParameterIDs::formantFollower, formantFollowerRelay, formantFollowerAttachment);
    attachSlider(ParameterIDs::focus, focusRelay, focusAttachment);
    attachSlider(ParameterIDs::reality, realityRelay, realityAttachment);
    attachSlider(ParameterIDs::sibilance, sibilanceRelay, sibilanceAttachment);
    attachSlider(ParameterIDs::mix, mixRelay, mixAttachment);
    attachSlider(ParameterIDs::outputGain, outputGainRelay, outputGainAttachment);

    if (auto* modeParam = audioProcessor.getAPVTS().getParameter(ParameterIDs::mode); modeParam != nullptr)
    {
        modeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
            *static_cast<juce::RangedAudioParameter*>(modeParam),
            modeRelay,
            nullptr);
    }
    else
    {
        jassertfalse;
    }

    if (auto* debugModeParam = audioProcessor.getAPVTS().getParameter(ParameterIDs::debugMode); debugModeParam != nullptr)
    {
        debugModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
            *static_cast<juce::RangedAudioParameter*>(debugModeParam),
            debugModeRelay,
            nullptr);
    }
    else
    {
        jassertfalse;
    }

    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    setSize(820, 500);
}

MorphantAudioProcessorEditor::~MorphantAudioProcessorEditor() = default;

void MorphantAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void MorphantAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource> MorphantAudioProcessorEditor::getResource(const juce::String& url)
{
    const auto path = url == "/"
        ? juce::String{"index.html"}
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
    else if (path == "js/index.js")
    {
        data = BinaryData::index_js;
        size = BinaryData::index_jsSize;
        mime = "text/javascript";
    }
    else if (path == "js/juce/index.js")
    {
        data = BinaryData::index_js2;
        size = BinaryData::index_js2Size;
        mime = "text/javascript";
    }
    else if (path == "js/juce/check_native_interop.js")
    {
        data = BinaryData::check_native_interop_js;
        size = BinaryData::check_native_interop_jsSize;
        mime = "text/javascript";
    }

    if (data == nullptr || size <= 0)
        return std::nullopt;

    std::vector<std::byte> bytes;
    bytes.resize(static_cast<size_t>(size));
    std::memcpy(bytes.data(), data, static_cast<size_t>(size));

    return juce::WebBrowserComponent::Resource{ std::move(bytes), juce::String{mime} };
}

