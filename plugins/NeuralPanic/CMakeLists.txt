cmake_minimum_required(VERSION 3.15)

juce_add_plugin(NeuralPanic
    COMPANY_NAME "N/A"
    PLUGIN_MANUFACTURER_CODE "Plgn"
    PLUGIN_CODE "NrPn"
    FORMATS VST3 Standalone
    PRODUCT_NAME "NeuralPanic"
    NEEDS_WEBVIEW2 TRUE
)

juce_add_binary_data(NeuralPanic_WebUI
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/app.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

target_sources(NeuralPanic
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(NeuralPanic
    PRIVATE
        Source
)

target_link_libraries(NeuralPanic
    PRIVATE
        NeuralPanic_WebUI
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(NeuralPanic
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)
