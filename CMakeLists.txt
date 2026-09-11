cmake_minimum_required(VERSION 3.22)

project(ReFeedback VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable Universal Binary on macOS builds
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "macOS deployment target" FORCE)
endif()

# Fetch JUCE via FetchContent
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(juce)

# ==============================================================================
# Plugin Target: Re-Feedback by Omega Dumpster
# ==============================================================================

set(PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

juce_add_plugin(ReFeedback
    COMPANY_NAME "Omega Dumpster"
    BUNDLE_ID "com.OmegaDumpster.ReFeedback"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Omeg
    PLUGIN_CODE Refb
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Re-Feedback"
    DESCRIPTION "Configurable Pure Acoustic & Electronic Feedback Synthesizer by Omega Dumpster"
    ICON_BIG "Resources/icon.png"
    ICON_SMALL "Resources/icon.png"
    PLIST_TO_MERGE "<plist><dict><key>NSMicrophoneUsageDescription</key><string>Re-Feedback requires microphone and audio input access to capture and model acoustic feedback from your instrument or voice.</string></dict></plist>"
)

juce_add_binary_data(ReFeedback_BinaryData
    SOURCES
        Resources/icon.png
)

target_sources(ReFeedback PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/DSP/PitchTracker.h
    Source/DSP/FractionalDelay.h
    Source/DSP/SafetyLimiter.h
    Source/DSP/FeedbackEngine.h
    Source/DSP/FeedbackEngine.cpp
    Source/GUI/CustomLookAndFeel.h
    Source/GUI/Visualizers.h
    Source/GUI/Visualizers.cpp
)

target_compile_definitions(ReFeedback PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ReFeedback PRIVATE
    ReFeedback_BinaryData
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ==============================================================================
# Standalone DSP Unit Tests
# ==============================================================================

option(BUILD_TESTS "Build DSP Unit Tests" ON)
if(BUILD_TESTS)
    add_executable(DSPTests
        Tests/DSPTests.cpp
        Source/DSP/FeedbackEngine.cpp
    )

    target_include_directories(DSPTests PRIVATE
        Source
        Source/DSP
    )

    target_compile_features(DSPTests PRIVATE cxx_std_20)
endif()
