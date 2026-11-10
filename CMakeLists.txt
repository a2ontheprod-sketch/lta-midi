cmake_minimum_required(VERSION 3.22)
project(LTAHarmony VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(LTA_PLUGIN_FORMATS VST3 AU)
else()
    set(LTA_PLUGIN_FORMATS VST3)
endif()

juce_add_plugin(LTAHarmony
    COMPANY_NAME "LISTENTOAARXN"
    BUNDLE_ID "com.listentoaarxn.ltaharmony"
    PLUGIN_MANUFACTURER_CODE Ltaa
    PLUGIN_CODE Lhar
    FORMATS ${LTA_PLUGIN_FORMATS}
    PRODUCT_NAME "LTA Harmony"
    PLUGIN_NAME "LTA Harmony"
    DESCRIPTION "Generative chord, melody and bass MIDI workstation"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Generator
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(LTAHarmony)

target_sources(LTAHarmony PRIVATE
    Source/MusicEngine.cpp
    Source/MidiWriter.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_include_directories(LTAHarmony PRIVATE Source)
target_compile_definitions(LTAHarmony PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_MODAL_LOOPS_PERMITTED=0
)

target_link_libraries(LTAHarmony PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags
)

include(CTest)
if(BUILD_TESTING)
    add_executable(LTAHarmonyEngineTests
        tests/EngineTests.cpp Source/MusicEngine.cpp Source/MidiWriter.cpp)
    target_include_directories(LTAHarmonyEngineTests PRIVATE Source)
    target_compile_features(LTAHarmonyEngineTests PRIVATE cxx_std_17)
    add_test(NAME EngineTests COMMAND LTAHarmonyEngineTests)
endif()
