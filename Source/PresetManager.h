#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace zx
{

/**
    Factory + user preset handling.

    Factory presets live in code as parameter snapshots.  User presets are XML
    files under the shared application-data folder, so they survive updates and
    are visible to every host on the machine.
*/
class PresetManager : public juce::ChangeBroadcaster
{
public:
    struct FactoryPreset
    {
        const char* name;
        std::vector<std::pair<const char*, float>> values;
    };

    explicit PresetManager (juce::AudioProcessorValueTreeState& stateToUse);

    static juce::File getUserPresetDirectory();

    /** Factory names followed by user names, in display order. */
    juce::StringArray getAllPresetNames() const;

    void loadPreset (const juce::String& name);
    void loadPresetAtIndex (int index);
    void nextPreset();
    void previousPreset();

    /** Writes the current parameter state out as a user preset. */
    bool savePreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);

    juce::String getCurrentPresetName() const { return currentPreset; }
    void setCurrentPresetName (const juce::String& name);

    int getCurrentIndex() const;
    bool isFactoryPreset (const juce::String& name) const;

    void refresh();

private:
    void applyFactory (const FactoryPreset& preset);

    juce::AudioProcessorValueTreeState& apvts;
    juce::String currentPreset { "Default" };
    juce::StringArray userPresets;

    static const std::vector<FactoryPreset> factoryPresets;
};

} // namespace zx
