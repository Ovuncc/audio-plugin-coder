#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"
#include <BinaryData.h>
#include <cstring>

static void logDebug(const juce::String& message)
{
    juce::File logFile(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDocumentsDirectory)
                       .getChildFile("Osmium_v2_Debug.log"));
    logFile.appendText(juce::Time::getCurrentTime().toString(true, true) + ": " + message + "\n");
}

OsmiumAudioProcessorEditor::OsmiumAudioProcessorEditor (OsmiumAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    logDebug("Editor Constructor - Start");

    auto userDataFolder = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory)
                          .getChildFile("Osmium_v2_WebView2_" + juce::String(juce::Time::getMillisecondCounter()));

    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options()
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(userDataFolder))
            .withNativeIntegrationEnabled()
            .withOptionsFrom(intensityRelay)
            .withOptionsFrom(outputGainRelay)
            .withOptionsFrom(bypassRelay)
            .withOptionsFrom(oversamplingRelay)
            .withOptionsFrom(processingModeRelay)
            .withResourceProvider([this](const auto& url) { return getResource(url); })
    );

    addAndMakeVisible(*webView);

    auto attachSlider = [this](const char* id, juce::WebSliderRelay& relay) {
        auto* parameter = audioProcessor.getAPVTS().getParameter(id);
        if (parameter == nullptr) {
            logDebug("ERROR: missing slider parameter " + juce::String(id));
            jassertfalse;
            return;
        }
        sliderAttachments.push_back(std::make_unique<juce::WebSliderParameterAttachment>(*parameter, relay, nullptr));
    };

    auto attachToggle = [this](const char* id, juce::WebToggleButtonRelay& relay) {
        auto* parameter = audioProcessor.getAPVTS().getParameter(id);
        if (parameter == nullptr) {
            logDebug("ERROR: missing toggle parameter " + juce::String(id));
            jassertfalse;
            return;
        }
        toggleAttachments.push_back(std::make_unique<juce::WebToggleButtonParameterAttachment>(*parameter, relay, nullptr));
    };

    auto attachChoice = [this](const char* id, juce::WebComboBoxRelay& relay) {
        auto* parameter = audioProcessor.getAPVTS().getParameter(id);
        if (parameter == nullptr) {
            logDebug("ERROR: missing choice parameter " + juce::String(id));
            jassertfalse;
            return;
        }
        comboAttachments.push_back(std::make_unique<juce::WebComboBoxParameterAttachment>(*parameter, relay, nullptr));
    };

    attachSlider(ParameterIDs::intensity, intensityRelay);
    attachSlider(ParameterIDs::outputGain, outputGainRelay);
    attachToggle(ParameterIDs::bypass, bypassRelay);
    attachChoice(ParameterIDs::oversamplingMode, oversamplingRelay);
    attachChoice(ParameterIDs::processingMode, processingModeRelay);

    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    setSize(500, 400);
    logDebug("Editor Constructor - End");
}

OsmiumAudioProcessorEditor::~OsmiumAudioProcessorEditor()
{
    logDebug("Editor Destructor");
}

void OsmiumAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void OsmiumAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource> OsmiumAudioProcessorEditor::getResource (const juce::String& url)
{
    const auto urlToRetrieve = url == "/" ? juce::String{ "index.html" }
                                          : url.fromFirstOccurrenceOf("/", false, false);

    const char* data = nullptr;
    int size = 0;
    const char* mime = "text/plain";

    if (urlToRetrieve == "index.html") {
        data = BinaryData::index_html;
        size = BinaryData::index_htmlSize;
        mime = "text/html";
    } else if (urlToRetrieve == "js/index.js") {
        data = BinaryData::index_js;
        size = BinaryData::index_jsSize;
        mime = "text/javascript";
    } else if (urlToRetrieve == "js/juce/index.js") {
        data = BinaryData::index_js2;
        size = BinaryData::index_js2Size;
        mime = "text/javascript";
    } else if (urlToRetrieve == "js/juce/check_native_interop.js") {
        data = BinaryData::check_native_interop_js;
        size = BinaryData::check_native_interop_jsSize;
        mime = "text/javascript";
    }

    if (data != nullptr && size > 0) {
        std::vector<std::byte> result;
        result.resize((size_t)size);
        std::memcpy(result.data(), data, (size_t)size);
        return juce::WebBrowserComponent::Resource{ std::move(result), juce::String{ mime } };
    }

    return std::nullopt;
}
