cmake_minimum_required(VERSION 3.22)

project(ArgentumPlate VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Pull in JUCE. If you already have a local JUCE checkout, you can instead
# comment out the FetchContent block and use:
#   add_subdirectory(C:/path/to/JUCE JUCE)
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ArgentumPlate
    COMPANY_NAME "Sharon Audio"
    PLUGIN_MANUFACTURER_CODE Shrn
    PLUGIN_CODE Apl1
    FORMATS VST3
    PRODUCT_NAME "Argentum Plate"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES "Fx" "Reverb"
)

juce_generate_juce_header(ArgentumPlate)

target_sources(ArgentumPlate PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_include_directories(ArgentumPlate PRIVATE Source)

target_compile_definitions(ArgentumPlate PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    _USE_MATH_DEFINES=1
)

target_link_libraries(ArgentumPlate PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
