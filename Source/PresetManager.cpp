#include "PresetManager.h"
#include "Parameters.h"

namespace zx
{

// ---------------------------------------------------------------------------
//  Factory presets.  Values are in *real* units — the manager converts them to
//  the normalised 0..1 range each parameter expects.
// ---------------------------------------------------------------------------
const std::vector<PresetManager::FactoryPreset> PresetManager::factoryPresets =
{
    { "Default", {
        { ParamID::input, 0.0f }, { ParamID::denoise, 0.0f }, { ParamID::lowCut, 20.0f }, { ParamID::highCut, 22000.0f },
        { ParamID::lowGain, 0.0f }, { ParamID::lowFreq, 100.0f }, { ParamID::lowShape, 1.0f },
        { ParamID::midGain, 0.0f }, { ParamID::midTone, 0.5f },
        { ParamID::highGain, 0.0f }, { ParamID::highMode, 0.0f },
        { ParamID::deEss, 0.0f }, { ParamID::deEssFreq, 6500.0f },
        { ParamID::control, 0.0f }, { ParamID::controlRange, 1.0f }, { ParamID::controlMode, 0.0f },
        { ParamID::push, 0.0f }, { ParamID::pushRange, 1.0f }, { ParamID::pushMode, 0.0f },
        { ParamID::saturate, 0.0f }, { ParamID::satMode, 0.0f },
        { ParamID::output, 0.0f }, { ParamID::limit, -0.3f }, { ParamID::limitOn, 1.0f },
        { ParamID::oversample, 1.0f }, { ParamID::mix, 1.0f } } },

    { "Modern Pop Lead", {
        { ParamID::input, 2.0f }, { ParamID::denoise, 0.15f }, { ParamID::lowCut, 95.0f }, { ParamID::highCut, 22000.0f },
        { ParamID::lowGain, -1.5f }, { ParamID::lowFreq, 220.0f }, { ParamID::lowShape, 0.0f },
        { ParamID::midGain, 1.5f }, { ParamID::midTone, 0.72f },
        { ParamID::highGain, 4.0f }, { ParamID::highMode, 0.0f },
        { ParamID::deEss, 0.45f }, { ParamID::deEssFreq, 7200.0f },
        { ParamID::control, 0.55f }, { ParamID::controlRange, 1.0f }, { ParamID::controlMode, 0.0f },
        { ParamID::push, 0.35f }, { ParamID::pushRange, 0.0f }, { ParamID::pushMode, 0.0f },
        { ParamID::saturate, 0.28f }, { ParamID::satMode, 0.0f },
        { ParamID::output, 0.0f }, { ParamID::limit, -0.3f }, { ParamID::limitOn, 1.0f },
        { ParamID::oversample, 2.0f }, { ParamID::mix, 1.0f } } },

    { "Warm Soul Vocal", {
        { ParamID::input, 1.0f }, { ParamID::denoise, 0.1f }, { ParamID::lowCut, 70.0f }, { ParamID::highCut, 18000.0f },
        { ParamID::lowGain, 2.5f }, { ParamID::lowFreq, 150.0f }, { ParamID::lowShape, 1.0f },
        { ParamID::midGain, -1.0f }, { ParamID::midTone, 0.35f },
        { ParamID::highGain, 2.0f }, { ParamID::highMode, 1.0f },
        { ParamID::deEss, 0.3f }, { ParamID::deEssFreq, 6000.0f },
        { ParamID::control, 0.45f }, { ParamID::controlRange, 1.0f }, { ParamID::controlMode, 1.0f },
        { ParamID::push, 0.25f }, { ParamID::pushRange, 0.0f }, { ParamID::pushMode, 1.0f },
        { ParamID::saturate, 0.4f }, { ParamID::satMode, 1.0f },
        { ParamID::output, 0.0f }, { ParamID::limit, -0.5f }, { ParamID::limitOn, 1.0f },
        { ParamID::oversample, 1.0f }, { ParamID::mix, 1.0f } } },

    { "Rap / Trap Vocal", {
        { ParamID::input, 3.0f }, { ParamID::denoise, 0.2f }, { ParamID::lowCut, 110.0f }, { ParamID::highCut, 22000.0f },
        { ParamID::lowGain, -2.0f }, { ParamID::lowFreq, 260.0f }, { ParamID::lowShape, 0.0f },
        { ParamID::midGain, 2.5f }, { ParamID::midTone, 0.6f },
        { ParamID::highGain, 3.0f }, { ParamID::highMode, 2.0f },
        { ParamID::deEss, 0.5f }, { ParamID::deEssFreq, 7500.0f },
        { ParamID::control, 0.7f }, { ParamID::controlRange, 2.0f }, { ParamID::controlMode, 0.0f },
        { ParamID::push, 0.6f }, { ParamID::pushRange, 1.0f }, { ParamID::pushMode, 2.0f },
        { ParamID::saturate, 0.45f }, { ParamID::satMode, 3.0f },
        { ParamID::output, -1.0f }, { ParamID::limit, -0.3f }, { ParamID::limitOn, 1.0f },
        { ParamID::oversample, 3.0f }, { ParamID::mix, 1.0f } } },

    { "Airy Backing Stack", {
        { ParamID::input, 0.0f }, { ParamID::denoise, 0.1f }, { ParamID::lowCut, 160.0f }, { ParamID::highCut, 22000.0f },
        { ParamID::lowGain, -3.0f }, { ParamID::lowFreq, 200.0f }, { ParamID::lowShape, 1.0f },
        { ParamID::midGain, -2.0f }, { ParamID::midTone, 0.5f },
        { ParamID::highGain, 5.5f }, { ParamID::highMode, 0.0f },
        { ParamID::deEss, 0.55f }, { ParamID::deEssFreq, 7000.0f },
        { ParamID::control, 0.4f }, { ParamID::controlRange, 0.0f }, { ParamID::controlMode, 1.0f },
        { ParamID::push, 0.2f }, { ParamID::pushRange, 0.0f }, { ParamID::pushMode, 0.0f },
        { ParamID::saturate, 0.15f }, { ParamID::satMode, 0.0f },
        { ParamID::output, 0.0f }, { ParamID::limit, -0.5f }, { ParamID::limitOn, 1.0f },
        { ParamID::oversample, 1.0f }, { ParamID::mix, 1.0f } } },

    { "Rock Shout", {
        { ParamID::input, 2.0f }, { ParamID::denoise, 0.05f }, { ParamID::lowCut, 105.0f }, { ParamID::highCut, 20000.0f },
        { ParamID::lowGain, 1.0f }, { ParamID::lowFreq, 130.0f }, { ParamID::lowShape, 1.0f },
        { ParamID::midGain, 3.5f }, { ParamID::midTone, 0.55f },
        { ParamID::highGain, 2.0f }, { ParamID::highMode, 2.0f },
        { ParamID::deEss, 0.35f }, { ParamID::deEssFreq, 6800.0f },
        { ParamID::control, 0.65f }, { ParamID::controlRange, 2.0f }, { ParamID::controlMode, 0.0f },
        { ParamID::push, 0.5f }, { ParamID::pushRange, 1.0f }, { ParamID::pushMode, 0.0f },
        { ParamID::saturate, 0.6f }, { ParamID::satMode, 2.0f },
        { ParamID::output, -1.5f }, { ParamID::limit, -0.3f }, { ParamID::limitOn, 1.0f },
        { ParamID::oversample, 2.0f }, { ParamID::mix, 1.0f } } },

    { "Podcast / Voice Over", {
        { ParamID::input, 4.0f }, { ParamID::denoise, 0.45f }, { ParamID::lowCut, 85.0f }, { ParamID::highCut, 16000.0f },
        { ParamID::lowGain, 1.5f }, { ParamID::lowFreq, 120.0f }, { ParamID::lowShape, 1.0f },
        { ParamID::midGain, 1.0f }, { ParamID::midTone, 0.42f },
        { ParamID::highGain, 2.5f }, { ParamID::highMode, 1.0f },
        { ParamID::deEss, 0.4f }, { ParamID::deEssFreq, 6200.0f },
        { ParamID::control, 0.6f }, { ParamID::controlRange, 1.0f }, { ParamID::controlMode, 2.0f },
        { ParamID::push, 0.4f }, { ParamID::pushRange, 0.0f }, { ParamID::pushMode, 1.0f },
        { ParamID::saturate, 0.2f }, { ParamID::satMode, 1.0f },
        { ParamID::output, 0.0f }, { ParamID::limit, -1.0f }, { ParamID::limitOn, 1.0f },
        { ParamID::oversample, 1.0f }, { ParamID::mix, 1.0f } } },

    { "Vintage Tape Vocal", {
        { ParamID::input, 1.0f }, { ParamID::denoise, 0.0f }, { ParamID::lowCut, 75.0f }, { ParamID::highCut, 14000.0f },
        { ParamID::lowGain, 2.0f }, { ParamID::lowFreq, 110.0f }, { ParamID::lowShape, 1.0f },
        { ParamID::midGain, 1.5f }, { ParamID::midTone, 0.3f },
        { ParamID::highGain, -1.0f }, { ParamID::highMode, 0.0f },
        { ParamID::deEss, 0.25f }, { ParamID::deEssFreq, 5800.0f },
        { ParamID::control, 0.5f }, { ParamID::controlRange, 1.0f }, { ParamID::controlMode, 1.0f },
        { ParamID::push, 0.3f }, { ParamID::pushRange, 0.0f }, { ParamID::pushMode, 1.0f },
        { ParamID::saturate, 0.7f }, { ParamID::satMode, 2.0f },
        { ParamID::output, 0.5f }, { ParamID::limit, -0.5f }, { ParamID::limitOn, 1.0f },
        { ParamID::oversample, 2.0f }, { ParamID::mix, 1.0f } } },

    { "Aggressive Ad-Lib", {
        { ParamID::input, 3.0f }, { ParamID::denoise, 0.15f }, { ParamID::lowCut, 140.0f }, { ParamID::highCut, 22000.0f },
        { ParamID::lowGain, -4.0f }, { ParamID::lowFreq, 240.0f }, { ParamID::lowShape, 1.0f },
        { ParamID::midGain, 4.0f }, { ParamID::midTone, 0.68f },
        { ParamID::highGain, 4.5f }, { ParamID::highMode, 0.0f },
        { ParamID::deEss, 0.6f }, { ParamID::deEssFreq, 7800.0f },
        { ParamID::control, 0.8f }, { ParamID::controlRange, 2.0f }, { ParamID::controlMode, 0.0f },
        { ParamID::push, 0.75f }, { ParamID::pushRange, 2.0f }, { ParamID::pushMode, 2.0f },
        { ParamID::saturate, 0.55f }, { ParamID::satMode, 0.0f },
        { ParamID::output, -2.0f }, { ParamID::limit, -0.3f }, { ParamID::limitOn, 1.0f },
        { ParamID::oversample, 3.0f }, { ParamID::mix, 1.0f } } },

    { "Gentle Polish", {
        { ParamID::input, 0.0f }, { ParamID::denoise, 0.2f }, { ParamID::lowCut, 60.0f }, { ParamID::highCut, 22000.0f },
        { ParamID::lowGain, 0.5f }, { ParamID::lowFreq, 120.0f }, { ParamID::lowShape, 1.0f },
        { ParamID::midGain, 0.5f }, { ParamID::midTone, 0.5f },
        { ParamID::highGain, 1.5f }, { ParamID::highMode, 0.0f },
        { ParamID::deEss, 0.2f }, { ParamID::deEssFreq, 6500.0f },
        { ParamID::control, 0.3f }, { ParamID::controlRange, 0.0f }, { ParamID::controlMode, 1.0f },
        { ParamID::push, 0.15f }, { ParamID::pushRange, 0.0f }, { ParamID::pushMode, 0.0f },
        { ParamID::saturate, 0.12f }, { ParamID::satMode, 0.0f },
        { ParamID::output, 0.0f }, { ParamID::limit, -0.5f }, { ParamID::limitOn, 1.0f },
        { ParamID::oversample, 1.0f }, { ParamID::mix, 1.0f } } },
};

// ---------------------------------------------------------------------------

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& stateToUse)
    : apvts (stateToUse)
{
    auto dir = getUserPresetDirectory();

    if (! dir.exists())
        dir.createDirectory();

    refresh();
}

juce::File PresetManager::getUserPresetDirectory()
{
    return juce::File::getSpecialLocation (juce::File::commonDocumentsDirectory)
               .getChildFile ("Zinox Audio")
               .getChildFile ("Zinox Vocals")
               .getChildFile ("Presets");
}

void PresetManager::refresh()
{
    userPresets.clear();

    auto dir = getUserPresetDirectory();

    if (dir.isDirectory())
    {
        for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.zxpreset"))
            userPresets.add (f.getFileNameWithoutExtension());
    }

    userPresets.sort (true);
    sendChangeMessage();
}

juce::StringArray PresetManager::getAllPresetNames() const
{
    juce::StringArray names;

    for (const auto& p : factoryPresets)
        names.add (p.name);

    names.addArray (userPresets);
    return names;
}

bool PresetManager::isFactoryPreset (const juce::String& name) const
{
    for (const auto& p : factoryPresets)
        if (name == p.name)
            return true;

    return false;
}

int PresetManager::getCurrentIndex() const
{
    return getAllPresetNames().indexOf (currentPreset);
}

void PresetManager::setCurrentPresetName (const juce::String& name)
{
    currentPreset = name;
    sendChangeMessage();
}

void PresetManager::applyFactory (const FactoryPreset& preset)
{
    for (const auto& [id, realValue] : preset.values)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (realValue));
    }
}

void PresetManager::loadPreset (const juce::String& name)
{
    for (const auto& p : factoryPresets)
    {
        if (name == p.name)
        {
            applyFactory (p);
            currentPreset = name;
            sendChangeMessage();
            return;
        }
    }

    auto file = getUserPresetDirectory().getChildFile (name + ".zxpreset");

    if (! file.existsAsFile())
        return;

    if (auto xml = juce::XmlDocument::parse (file))
    {
        auto tree = juce::ValueTree::fromXml (*xml);

        if (tree.isValid())
        {
            // Copy value-by-value rather than replacing the whole state, so the
            // host keeps its automation connections intact.
            for (auto* param : apvts.processor.getParameters())
            {
                if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
                {
                    auto child = tree.getChildWithProperty ("id", p->paramID);

                    if (child.isValid())
                        p->setValueNotifyingHost (p->convertTo0to1 ((float) child.getProperty ("value")));
                }
            }

            currentPreset = name;
            sendChangeMessage();
        }
    }
}

void PresetManager::loadPresetAtIndex (int index)
{
    auto names = getAllPresetNames();

    if (juce::isPositiveAndBelow (index, names.size()))
        loadPreset (names[index]);
}

void PresetManager::nextPreset()
{
    auto names = getAllPresetNames();

    if (names.isEmpty())
        return;

    const auto idx = juce::jmax (0, getCurrentIndex());
    loadPresetAtIndex ((idx + 1) % names.size());
}

void PresetManager::previousPreset()
{
    auto names = getAllPresetNames();

    if (names.isEmpty())
        return;

    const auto idx = juce::jmax (0, getCurrentIndex());
    loadPresetAtIndex ((idx - 1 + names.size()) % names.size());
}

bool PresetManager::savePreset (const juce::String& name)
{
    if (name.isEmpty())
        return false;

    juce::ValueTree tree { "ZINOX_PRESET" };
    tree.setProperty ("name", name, nullptr);

    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            juce::ValueTree child { "PARAM" };
            child.setProperty ("id", p->paramID, nullptr);
            child.setProperty ("value", p->convertFrom0to1 (p->getValue()), nullptr);
            tree.appendChild (child, nullptr);
        }
    }

    auto dir = getUserPresetDirectory();

    if (! dir.exists())
        dir.createDirectory();

    auto file = dir.getChildFile (name + ".zxpreset");

    if (auto xml = tree.createXml())
    {
        if (xml->writeTo (file))
        {
            currentPreset = name;
            refresh();
            return true;
        }
    }

    return false;
}

bool PresetManager::deleteUserPreset (const juce::String& name)
{
    if (isFactoryPreset (name))
        return false;

    auto file = getUserPresetDirectory().getChildFile (name + ".zxpreset");

    if (file.existsAsFile() && file.deleteFile())
    {
        if (currentPreset == name)
            currentPreset = "Default";

        refresh();
        return true;
    }

    return false;
}

} // namespace zx
