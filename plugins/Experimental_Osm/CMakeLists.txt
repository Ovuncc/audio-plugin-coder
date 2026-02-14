cmake_minimum_required(VERSION 3.15)

# Plugin definition with WebView2 support
juce_add_plugin(experimental_osm
    COMPANY_NAME "N/A"
    PLUGIN_MANUFACTURER_CODE "Plgn"
    PLUGIN_CODE "ExOm"
    FORMATS VST3 Standalone
    PRODUCT_NAME "Experimental_Osm"
    NEEDS_WEBVIEW2 TRUE
)

# CRITICAL: Embed web UI files into binary
# This creates a zip file and embeds it as C++ binary data
juce_add_binary_data(experimental_osm_WebUI
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/index.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
    # Add any additional web assets here:
    # Source/ui/public/css/style.css
)

# Source files
target_sources(experimental_osm
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(experimental_osm
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(experimental_osm
    PRIVATE
        experimental_osm_WebUI
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

# CRITICAL: WebView2 compile definitions
target_compile_definitions(experimental_osm
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)
