#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"
#include <BinaryData.h>
#include <cstring>

static void logDebug(const juce::String& message)
{
    juce::File logFile(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDocumentsDirectory)
                       .getChildFile("Experimental_Osmium_v3_Debug.log"));
    logFile.appendText(juce::Time::getCurrentTime().toString(true, true) + ": " + message + "\n");
}

OsmiumAudioProcessorEditor::OsmiumAudioProcessorEditor (OsmiumAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    logDebug("Editor Constructor - Start");

    auto userDataFolder = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory)
                          .getChildFile("Experimental_Osmium_v3_WebView2_" + juce::String(juce::Time::getMillisecondCounter()));

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
            .withOptionsFrom(expManualModeRelay)
            .withOptionsFrom(expMuteLowBandRelay)
            .withOptionsFrom(expMuteHighBandRelay)
            .withOptionsFrom(expCrossoverRelay)
            .withOptionsFrom(expOversamplingRelay)
            .withOptionsFrom(expProcessingModeRelay)
            .withOptionsFrom(expBypassLowTransientRelay)
            .withOptionsFrom(expBypassLowSaturationRelay)
            .withOptionsFrom(expBypassLowLimiterRelay)
            .withOptionsFrom(expBypassHighTransientRelay)
            .withOptionsFrom(expBypassHighSaturationRelay)
            .withOptionsFrom(expBypassTightnessRelay)
            .withOptionsFrom(expBypassOutputLimiterRelay)
            .withOptionsFrom(expLowAttackBoostRelay)
            .withOptionsFrom(expLowAttackTimeRelay)
            .withOptionsFrom(expLowPostCompRelay)
            .withOptionsFrom(expLowCompTimeRelay)
            .withOptionsFrom(expLowBodyLiftRelay)
            .withOptionsFrom(expLowSatMixRelay)
            .withOptionsFrom(expLowSatDriveRelay)
            .withOptionsFrom(expLowWaveShaperRelay)
            .withOptionsFrom(expLowFastAttackRelay)
            .withOptionsFrom(expLowFastReleaseRelay)
            .withOptionsFrom(expLowSlowReleaseRelay)
            .withOptionsFrom(expLowAttackGainSmoothRelay)
            .withOptionsFrom(expLowCompAttackRatioRelay)
            .withOptionsFrom(expLowCompGainSmoothRelay)
            .withOptionsFrom(expLowTransientSensitivityRelay)
            .withOptionsFrom(expLowLimiterThresholdRelay)
            .withOptionsFrom(expLowLimiterReleaseRelay)
            .withOptionsFrom(expHighAttackBoostRelay)
            .withOptionsFrom(expHighAttackTimeRelay)
            .withOptionsFrom(expHighPostCompRelay)
            .withOptionsFrom(expHighCompTimeRelay)
            .withOptionsFrom(expHighDriveRelay)
            .withOptionsFrom(expHighSatMixRelay)
            .withOptionsFrom(expHighSatDriveRelay)
            .withOptionsFrom(expHighTrimRelay)
            .withOptionsFrom(expHighWaveShaperRelay)
            .withOptionsFrom(expHighFastAttackRelay)
            .withOptionsFrom(expHighFastReleaseRelay)
            .withOptionsFrom(expHighSlowReleaseRelay)
            .withOptionsFrom(expHighAttackGainSmoothRelay)
            .withOptionsFrom(expHighCompAttackRatioRelay)
            .withOptionsFrom(expHighCompGainSmoothRelay)
            .withOptionsFrom(expHighTransientSensitivityRelay)
            .withOptionsFrom(expTightSustainDepthRelay)
            .withOptionsFrom(expTightSustainAttackRelay)
            .withOptionsFrom(expTightReleaseRelay)
            .withOptionsFrom(expTightBellFreqRelay)
            .withOptionsFrom(expTightBellCutRelay)
            .withOptionsFrom(expTightBellQRelay)
            .withOptionsFrom(expTightHighShelfCutRelay)
            .withOptionsFrom(expTightHighShelfFreqRelay)
            .withOptionsFrom(expTightHighShelfQRelay)
            .withOptionsFrom(expTightFastAttackRelay)
            .withOptionsFrom(expTightFastReleaseRelay)
            .withOptionsFrom(expTightSlowAttackRelay)
            .withOptionsFrom(expTightSlowReleaseRelay)
            .withOptionsFrom(expTightProgramAttackRelay)
            .withOptionsFrom(expTightProgramReleaseRelay)
            .withOptionsFrom(expTightTransientSensitivityRelay)
            .withOptionsFrom(expTightThresholdOffsetRelay)
            .withOptionsFrom(expTightThresholdRangeRelay)
            .withOptionsFrom(expTightThresholdFloorRelay)
            .withOptionsFrom(expTightThresholdCeilRelay)
            .withOptionsFrom(expTightSustainCurveRelay)
            .withOptionsFrom(expOutputAutoMakeupRelay)
            .withOptionsFrom(expOutputBodyHoldRelay)
            .withOptionsFrom(expLimiterThresholdRelay)
            .withOptionsFrom(expLimiterReleaseRelay)
            .withNativeFunction("resetAllParameters", [this](const auto&, auto complete)
            {
                audioProcessor.resetAllParametersToDefaults();
                complete(true);
            })
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
    attachToggle(ParameterIDs::expManualMode, expManualModeRelay);
    attachToggle(ParameterIDs::expMuteLowBand, expMuteLowBandRelay);
    attachToggle(ParameterIDs::expMuteHighBand, expMuteHighBandRelay);

    attachSlider(ParameterIDs::expCrossoverHz, expCrossoverRelay);
    attachChoice(ParameterIDs::expOversamplingMode, expOversamplingRelay);
    attachChoice(ParameterIDs::expProcessingMode, expProcessingModeRelay);
    attachToggle(ParameterIDs::expBypassLowTransient, expBypassLowTransientRelay);
    attachToggle(ParameterIDs::expBypassLowSaturation, expBypassLowSaturationRelay);
    attachToggle(ParameterIDs::expBypassLowLimiter, expBypassLowLimiterRelay);
    attachToggle(ParameterIDs::expBypassHighTransient, expBypassHighTransientRelay);
    attachToggle(ParameterIDs::expBypassHighSaturation, expBypassHighSaturationRelay);
    attachToggle(ParameterIDs::expBypassTightness, expBypassTightnessRelay);
    attachToggle(ParameterIDs::expBypassOutputLimiter, expBypassOutputLimiterRelay);

    attachSlider(ParameterIDs::expLowAttackBoostDb, expLowAttackBoostRelay);
    attachSlider(ParameterIDs::expLowAttackTimeMs, expLowAttackTimeRelay);
    attachSlider(ParameterIDs::expLowPostCompDb, expLowPostCompRelay);
    attachSlider(ParameterIDs::expLowCompTimeMs, expLowCompTimeRelay);
    attachSlider(ParameterIDs::expLowBodyLiftDb, expLowBodyLiftRelay);
    attachSlider(ParameterIDs::expLowSatMix, expLowSatMixRelay);
    attachSlider(ParameterIDs::expLowSatDrive, expLowSatDriveRelay);
    attachChoice(ParameterIDs::expLowWaveShaperType, expLowWaveShaperRelay);
    attachSlider(ParameterIDs::expLowFastAttackMs, expLowFastAttackRelay);
    attachSlider(ParameterIDs::expLowFastReleaseMs, expLowFastReleaseRelay);
    attachSlider(ParameterIDs::expLowSlowReleaseMs, expLowSlowReleaseRelay);
    attachSlider(ParameterIDs::expLowAttackGainSmoothMs, expLowAttackGainSmoothRelay);
    attachSlider(ParameterIDs::expLowCompAttackRatio, expLowCompAttackRatioRelay);
    attachSlider(ParameterIDs::expLowCompGainSmoothMs, expLowCompGainSmoothRelay);
    attachSlider(ParameterIDs::expLowTransientSensitivity, expLowTransientSensitivityRelay);
    attachSlider(ParameterIDs::expLowLimiterThresholdDb, expLowLimiterThresholdRelay);
    attachSlider(ParameterIDs::expLowLimiterReleaseMs, expLowLimiterReleaseRelay);

    attachSlider(ParameterIDs::expHighAttackBoostDb, expHighAttackBoostRelay);
    attachSlider(ParameterIDs::expHighAttackTimeMs, expHighAttackTimeRelay);
    attachSlider(ParameterIDs::expHighPostCompDb, expHighPostCompRelay);
    attachSlider(ParameterIDs::expHighCompTimeMs, expHighCompTimeRelay);
    attachSlider(ParameterIDs::expHighDriveDb, expHighDriveRelay);
    attachSlider(ParameterIDs::expHighSatMix, expHighSatMixRelay);
    attachSlider(ParameterIDs::expHighSatDrive, expHighSatDriveRelay);
    attachSlider(ParameterIDs::expHighTrimDb, expHighTrimRelay);
    attachChoice(ParameterIDs::expHighWaveShaperType, expHighWaveShaperRelay);
    attachSlider(ParameterIDs::expHighFastAttackMs, expHighFastAttackRelay);
    attachSlider(ParameterIDs::expHighFastReleaseMs, expHighFastReleaseRelay);
    attachSlider(ParameterIDs::expHighSlowReleaseMs, expHighSlowReleaseRelay);
    attachSlider(ParameterIDs::expHighAttackGainSmoothMs, expHighAttackGainSmoothRelay);
    attachSlider(ParameterIDs::expHighCompAttackRatio, expHighCompAttackRatioRelay);
    attachSlider(ParameterIDs::expHighCompGainSmoothMs, expHighCompGainSmoothRelay);
    attachSlider(ParameterIDs::expHighTransientSensitivity, expHighTransientSensitivityRelay);

    attachSlider(ParameterIDs::expTightSustainDepthDb, expTightSustainDepthRelay);
    attachSlider(ParameterIDs::expTightSustainAttackMs, expTightSustainAttackRelay);
    attachSlider(ParameterIDs::expTightReleaseMs, expTightReleaseRelay);
    attachSlider(ParameterIDs::expTightBellFreqHz, expTightBellFreqRelay);
    attachSlider(ParameterIDs::expTightBellCutDb, expTightBellCutRelay);
    attachSlider(ParameterIDs::expTightBellQ, expTightBellQRelay);
    attachSlider(ParameterIDs::expTightHighShelfCutDb, expTightHighShelfCutRelay);
    attachSlider(ParameterIDs::expTightHighShelfFreqHz, expTightHighShelfFreqRelay);
    attachSlider(ParameterIDs::expTightHighShelfQ, expTightHighShelfQRelay);
    attachSlider(ParameterIDs::expTightFastAttackMs, expTightFastAttackRelay);
    attachSlider(ParameterIDs::expTightFastReleaseMs, expTightFastReleaseRelay);
    attachSlider(ParameterIDs::expTightSlowAttackMs, expTightSlowAttackRelay);
    attachSlider(ParameterIDs::expTightSlowReleaseMs, expTightSlowReleaseRelay);
    attachSlider(ParameterIDs::expTightProgramAttackMs, expTightProgramAttackRelay);
    attachSlider(ParameterIDs::expTightProgramReleaseMs, expTightProgramReleaseRelay);
    attachSlider(ParameterIDs::expTightTransientSensitivity, expTightTransientSensitivityRelay);
    attachSlider(ParameterIDs::expTightThresholdOffsetDb, expTightThresholdOffsetRelay);
    attachSlider(ParameterIDs::expTightThresholdRangeDb, expTightThresholdRangeRelay);
    attachSlider(ParameterIDs::expTightThresholdFloorDb, expTightThresholdFloorRelay);
    attachSlider(ParameterIDs::expTightThresholdCeilDb, expTightThresholdCeilRelay);
    attachSlider(ParameterIDs::expTightSustainCurve, expTightSustainCurveRelay);

    attachSlider(ParameterIDs::expOutputAutoMakeupDb, expOutputAutoMakeupRelay);
    attachSlider(ParameterIDs::expOutputBodyHoldDb, expOutputBodyHoldRelay);

    attachSlider(ParameterIDs::expLimiterThresholdDb, expLimiterThresholdRelay);
    attachSlider(ParameterIDs::expLimiterReleaseMs, expLimiterReleaseRelay);

    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    setSize(1540, 860);
    startTimerHz(30);
    logDebug("Editor Constructor - End");
}

OsmiumAudioProcessorEditor::~OsmiumAudioProcessorEditor()
{
    stopTimer();
    logDebug("Editor Destructor");
}

void OsmiumAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    const auto meters = audioProcessor.getMeterSnapshot();
    juce::var meterPayload(new juce::DynamicObject());

    if (auto* payload = meterPayload.getDynamicObject())
    {
        payload->setProperty("inputDb", meters.inputDb);
        payload->setProperty("outputDb", meters.outputDb);
        payload->setProperty("reductionDb", meters.reductionDb);
    }

    webView->emitEventIfBrowserIsVisible("exp_meter_update", meterPayload);
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
