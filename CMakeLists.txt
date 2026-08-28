cmake_minimum_required(VERSION 3.22)

project(ZinoxVocals VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version" FORCE)
endif()

if (MSVC)
    add_compile_options(/MP)          # parallel compile
endif()

# ---------------------------------------------------------------------------
# JUCE — fetched automatically. No manual download required.
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY  https://github.com/juce-framework/JUCE.git
    GIT_TAG         8.0.6
    GIT_SHALLOW     TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
set(ZINOX_FORMATS VST3 Standalone)
if (APPLE)
    list(APPEND ZINOX_FORMATS AU)
endif()

juce_add_plugin(ZinoxVocals
    COMPANY_NAME                "Zinox Audio"
    COMPANY_WEBSITE             "https://zinoxaudio.com"
    COMPANY_EMAIL               "support@zinoxaudio.com"
    BUNDLE_ID                   com.zinoxaudio.zinoxvocals
    PLUGIN_MANUFACTURER_CODE    Znxa
    PLUGIN_CODE                 Zvcl
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    FORMATS                     ${ZINOX_FORMATS}
    PRODUCT_NAME                "Zinox Vocals"
    VST3_CATEGORIES             "Fx" "Dynamics" "EQ"
    AU_MAIN_TYPE                kAudioUnitType_Effect
)

juce_generate_juce_header(ZinoxVocals)

target_sources(ZinoxVocals PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/PresetManager.cpp
    Source/Licensing.cpp
    Source/Trial.cpp
    Source/OfflineRenderer.cpp
    Source/GUI/ZinoxLookAndFeel.cpp
    Source/GUI/Widgets.cpp
)

target_include_directories(ZinoxVocals PRIVATE Source)

target_compile_definitions(ZinoxVocals PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_link_libraries(ZinoxVocals PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_cryptography
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------------
# License key generator - a small internal tool, not shipped to customers.
# Run it from the build directory: ZinoxLicenseGen genkeys / issue ...
# See Source/Tools/LicenseKeyGen.cpp for details.
# ---------------------------------------------------------------------------
add_executable(ZinoxLicenseGen Source/Tools/LicenseKeyGen.cpp Source/Licensing.cpp)
target_link_libraries(ZinoxLicenseGen PRIVATE juce::juce_core juce::juce_cryptography)
