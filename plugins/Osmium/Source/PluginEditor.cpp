#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"
#include <BinaryData.h>

// Debug Logger
static void logDebug(const juce::String& message)
{
    juce::File logFile(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDocumentsDirectory)
                       .getChildFile("Osmium_Debug.log"));
    logFile.appendText(juce::Time::getCurrentTime().toString(true, true) + ": " + message + "\n");
}

OsmiumAudioProcessorEditor::OsmiumAudioProcessorEditor (OsmiumAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    logDebug("Editor Constructor - Start");

    // CRITICAL: Create WebBrowserComponent
    logDebug("Creating WebView...");
    
    // Unique user data folder to prevent conflicts
    auto userDataFolder = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory)
                          .getChildFile("Osmium_WebView2_Data_" + juce::String(juce::Time::getMillisecondCounter()));
    
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
            .withResourceProvider([this](const auto& url) {
                logDebug("Resource Requested: " + url);
                return getResource(url);
            })
    );
    logDebug("WebView Created");

    addAndMakeVisible(*webView);
    logDebug("WebView Visible");

    // CRITICAL: Create parameter attachments AFTER WebBrowserComponent
    // Intensity (Slider)
    auto* intensityParam = audioProcessor.getAPVTS().getParameter(ParameterIDs::intensity);
    if (intensityParam == nullptr)
    {
        logDebug("ERROR: intensity parameter is NULL!");
        jassertfalse; // Development assertion
    }
    else
    {
        intensityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
            *intensityParam,
            intensityRelay,
            nullptr // UndoManager
        );
        logDebug("Intensity Attachment Created");
    }

    // Output Gain (Slider)
    auto* outputGainParam = audioProcessor.getAPVTS().getParameter(ParameterIDs::outputGain);
    if (outputGainParam == nullptr)
    {
        logDebug("ERROR: outputGain parameter is NULL!");
        jassertfalse;
    }
    else
    {
        outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
            *outputGainParam,
            outputGainRelay,
            nullptr
        );
        logDebug("Output Gain Attachment Created");
    }

    // Bypass (Toggle)
    auto* bypassParam = audioProcessor.getAPVTS().getParameter(ParameterIDs::bypass);
    if (bypassParam == nullptr)
    {
        logDebug("ERROR: bypass parameter is NULL!");
        jassertfalse;
    }
    else
    {
        bypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
            *bypassParam,
            bypassRelay,
            nullptr
        );
        logDebug("Bypass Attachment Created");
    }

    // CRITICAL: Load embedded web content
    logDebug("Navigating to URL...");
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    
    // Set window size LAST to ensure everything is initialized before resized() is called
    setSize (500, 400);
    logDebug("setSize complete");
    logDebug("Editor Constructor - End");
}

OsmiumAudioProcessorEditor::~OsmiumAudioProcessorEditor()
{
    logDebug("Editor Destructor");
}

//==============================================================================
void OsmiumAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void OsmiumAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds(getLocalBounds());
}

//==============================================================================
// Resource Provider Implementation

// Resource Provider Implementation
std::optional<juce::WebBrowserComponent::Resource> OsmiumAudioProcessorEditor::getResource (const juce::String& url)
{
    const auto urlToRetrieve = url == "/" ? juce::String{ "index.html" }
                                          : url.fromFirstOccurrenceOf("/", false, false);

    const char* data = nullptr;
    int size = 0;
    const char* mime = "text/plain";

    // Manual mapping of URL paths to BinaryData variables
    if (urlToRetrieve == "index.html")
    {
        data = BinaryData::index_html;
        size = BinaryData::index_htmlSize;
        mime = "text/html";
    }
    else if (urlToRetrieve == "js/index.js")
    {
        data = BinaryData::index_js;
        size = BinaryData::index_jsSize;
        mime = "text/javascript";
    }
    else if (urlToRetrieve == "js/juce/index.js")
    {
        data = BinaryData::index_js2;
        size = BinaryData::index_js2Size;
        mime = "text/javascript";
    }
    else if (urlToRetrieve == "js/juce/check_native_interop.js")
    {
        data = BinaryData::check_native_interop_js;
        size = BinaryData::check_native_interop_jsSize;
        mime = "text/javascript";
    }

    if (data != nullptr && size > 0)
    {
        std::vector<std::byte> result;
        result.resize(size);
        std::memcpy(result.data(), data, size);
        return juce::WebBrowserComponent::Resource{ std::move(result), juce::String{ mime } };
    }

    return std::nullopt;
}

static const char* getMimeForExtension (const juce::String& extension)
{
    static const std::unordered_map<juce::String, const char*> mimeMap =
    {
        { { "htm"   },  "text/html"                },
        { { "html"  },  "text/html"                },
        { { "js"    },  "text/javascript"          },
        { { "css"   },  "text/css"                 },
        { { "svg"   },  "image/svg+xml"            },
        { { "png"   },  "image/png"                },
        { { "jpg"   },  "image/jpeg"               },
        { { "jpeg"  },  "image/jpeg"               }
    };

    if (const auto it = mimeMap.find(extension.toLowerCase()); it != mimeMap.end())
        return it->second;

    return "text/plain";
}

static juce::String getExtension (juce::String filename)
{
    return filename.fromLastOccurrenceOf(".", false, false);
}

// Helper functions removed as they are no longer needed
// (getZipFile, streamToVector, etc.)
