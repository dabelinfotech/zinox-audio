#include "Parameters.h"

namespace zx
{

using APVTS  = juce::AudioProcessorValueTreeState;
using Float  = juce::AudioParameterFloat;
using Choice = juce::AudioParameterChoice;
using Bool   = juce::AudioParameterBool;

namespace
{
    juce::NormalisableRange<float> logRange (float lo, float hi, float interval = 0.0f)
    {
        juce::NormalisableRange<float> r { lo, hi,
            [] (float start, float end, float norm)
            {
                return start * std::pow (end / start, norm);
            },
            [] (float start, float end, float value)
            {
                return std::log (value / start) / std::log (end / start);
            },
            [interval] (float start, float end, float value)
            {
                const auto v = juce::jlimit (start, end, value);
                return interval > 0.0f ? start + interval * std::floor ((v - start) / interval + 0.5f) : v;
            } };
        return r;
    }

    juce::String hzText (float v, int)
    {
        return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz"
                            : juce::String (juce::roundToInt (v)) + " Hz";
    }

    juce::String dbText (float v, int)
    {
        return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB";
    }

    juce::String pctText (float v, int)
    {
        return juce::String (juce::roundToInt (v * 100.0f)) + " %";
    }
}

APVTS::ParameterLayout createParameterLayout()
{
    APVTS::ParameterLayout layout;

    // Bumping this invalidates saved automation for changed parameters, so it
    // stays at 1 unless a parameter's meaning actually changes.
    const int ver = 1;

    // ---- input stage -------------------------------------------------------
    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::input, ver }, "Input",
                                         juce::NormalisableRange<float> { -24.0f, 24.0f, 0.1f }, 0.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::denoise, ver }, "Denoise",
                                         juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctText)));

    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::lowCut, ver }, "Low Cut",
                                         logRange (20.0f, 500.0f), 20.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                                             [] (float x, int) { return x <= 20.5f ? juce::String ("OFF") : hzText (x, 0); })));

    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::highCut, ver }, "High Cut",
                                         logRange (2000.0f, 22000.0f), 22000.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                                             [] (float x, int) { return x >= 21500.0f ? juce::String ("OFF") : hzText (x, 0); })));

    // ---- tone stage --------------------------------------------------------
    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::lowGain, ver }, "Low",
                                         juce::NormalisableRange<float> { -12.0f, 12.0f, 0.1f }, 0.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::lowFreq, ver }, "Low Freq",
                                         logRange (40.0f, 320.0f), 100.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (hzText)));

    layout.add (std::make_unique<Choice> (juce::ParameterID { ParamID::lowShape, ver }, "Low Shape",
                                          Choices::lowShape, 1));

    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::midGain, ver }, "Mid",
                                         juce::NormalisableRange<float> { -12.0f, 12.0f, 0.1f }, 0.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::midTone, ver }, "Tone",
                                         juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.5f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                                             [] (float x, int)
                                             {
                                                 const auto f = 350.0f * std::pow (5000.0f / 350.0f, x);
                                                 return hzText (f, 0);
                                             })));

    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::highGain, ver }, "High",
                                         juce::NormalisableRange<float> { -12.0f, 12.0f, 0.1f }, 0.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<Choice> (juce::ParameterID { ParamID::highMode, ver }, "High Mode",
                                          Choices::highMode, 0));

    // ---- de-esser ----------------------------------------------------------
    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::deEss, ver }, "De-Ess",
                                         juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctText)));

    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::deEssFreq, ver }, "De-Ess Freq",
                                         logRange (3000.0f, 12000.0f), 6500.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (hzText)));

    // ---- dynamics ----------------------------------------------------------
    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::control, ver }, "Control",
                                         juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctText)));

    layout.add (std::make_unique<Choice> (juce::ParameterID { ParamID::controlRange, ver }, "Control Range",
                                          Choices::range, 1));

    layout.add (std::make_unique<Choice> (juce::ParameterID { ParamID::controlMode, ver }, "Control Mode",
                                          Choices::controlMode, 0));

    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::push, ver }, "Push",
                                         juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctText)));

    layout.add (std::make_unique<Choice> (juce::ParameterID { ParamID::pushRange, ver }, "Push Range",
                                          Choices::range, 1));

    layout.add (std::make_unique<Choice> (juce::ParameterID { ParamID::pushMode, ver }, "Push Mode",
                                          Choices::pushMode, 0));

    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::saturate, ver }, "Saturate",
                                         juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctText)));

    layout.add (std::make_unique<Choice> (juce::ParameterID { ParamID::satMode, ver }, "Saturate Mode",
                                          Choices::satMode, 0));

    // ---- output ------------------------------------------------------------
    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::output, ver }, "Output",
                                         juce::NormalisableRange<float> { -24.0f, 24.0f, 0.1f }, 0.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::limit, ver }, "Limit",
                                         juce::NormalisableRange<float> { -12.0f, 0.0f, 0.1f }, -0.3f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<Bool> (juce::ParameterID { ParamID::limitOn, ver }, "Limiter On", true));

    layout.add (std::make_unique<Float> (juce::ParameterID { ParamID::mix, ver }, "Mix",
                                         juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 1.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctText)));

    layout.add (std::make_unique<Choice> (juce::ParameterID { ParamID::oversample, ver }, "Oversampling",
                                          Choices::oversample, 1));

    layout.add (std::make_unique<Bool> (juce::ParameterID { ParamID::bypass, ver }, "Bypass", false));

    return layout;
}

} // namespace zx
