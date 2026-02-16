cmake_minimum_required(VERSION 3.15)

set(EXPERIMENTAL_OSM_REQUIRED_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/public/index.html"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/public/js/index.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/public/js/juce/index.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/public/js/juce/check_native_interop.js"
)

set(EXPERIMENTAL_OSM_MISSING_FILE "")
foreach(_file IN LISTS EXPERIMENTAL_OSM_REQUIRED_FILES)
    if(NOT EXISTS "${_file}")
        set(EXPERIMENTAL_OSM_MISSING_FILE "${_file}")
        break()
    endif()
endforeach()

if(NOT EXPERIMENTAL_OSM_MISSING_FILE STREQUAL "")
    message(STATUS "Experimental_Osm: Incomplete plugin snapshot, skipping (${EXPERIMENTAL_OSM_MISSING_FILE})")
    return()
endif()

juce_add_plugin(experimental_osm
    COMPANY_NAME "N/A"
    PLUGIN_MANUFACTURER_CODE "Plgn"
    PLUGIN_CODE "ExOm"
    FORMATS VST3 Standalone
    PRODUCT_NAME "Experimental_Osm"
    NEEDS_WEBVIEW2 TRUE
)

juce_add_binary_data(experimental_osm_WebUI
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/index.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

target_sources(experimental_osm
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(experimental_osm
    PRIVATE
        Source
)

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

target_compile_definitions(experimental_osm
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)
