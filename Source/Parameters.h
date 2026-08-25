#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace zx
{

// ---------------------------------------------------------------------------
//  Parameter identifiers.  Kept in one place so the processor, the editor and
//  the preset manager can never drift apart.
// ---------------------------------------------------------------------------
namespace ParamID
{
    inline constexpr auto input        = "input";
    inline constexpr auto denoise      = "denoise";
    inline constexpr auto lowCut       = "lowcut";
    inline constexpr auto highCut      = "highcut";

    inline constexpr auto lowGain      = "low_gain";
    inline constexpr auto lowFreq      = "low_freq";
    inline constexpr auto lowShape     = "low_shape";      // 0 = Peak, 1 = Shelf

    inline constexpr auto midGain      = "mid_gain";
    inline constexpr auto midTone      = "mid_tone";       // sweeps the mid centre

    inline constexpr auto highGain     = "high_gain";
    inline constexpr auto highMode     = "high_mode";      // Air / Bright / Presence

    inline constexpr auto deEss        = "deess";
    inline constexpr auto deEssFreq    = "deess_freq";

    inline constexpr auto control      = "control";        // main compressor amount
    inline constexpr auto controlRange = "control_range";  // -3 / -6 / -9 dB
    inline constexpr auto controlMode  = "control_mode";   // Aggro / Smooth / Vocal

    inline constexpr auto push         = "push";           // parallel density
    inline constexpr auto pushRange    = "push_range";
    inline constexpr auto pushMode     = "push_mode";      // Punch / Fat / Tight

    inline constexpr auto saturate     = "saturate";
    inline constexpr auto satMode      = "sat_mode";       // Rich / Warm / Tape / Tube

    inline constexpr auto output       = "output";
    inline constexpr auto limit        = "limit";          // ceiling, dB
    inline constexpr auto limitOn      = "limit_on";

    inline constexpr auto oversample   = "oversample";     // Off / 2x / 4x / 8x
    inline constexpr auto bypass       = "bypass";
    inline constexpr auto mix          = "mix";
}

// Choice strings — the editor reads these so the combo boxes always match.
namespace Choices
{
    inline const juce::StringArray lowShape    { "PEAK", "SHELF" };
    inline const juce::StringArray highMode    { "AIR", "BRIGHT", "PRESENCE" };
    inline const juce::StringArray controlMode { "AGGRO", "SMOOTH", "VOCAL" };
    inline const juce::StringArray pushMode    { "PUNCH", "FAT", "TIGHT" };
    inline const juce::StringArray satMode     { "RICH", "WARM", "TAPE", "TUBE" };
    inline const juce::StringArray range       { "-3", "-6", "-9" };
    inline const juce::StringArray oversample  { "OFF", "2X", "4X", "8X" };
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Maps the -3/-6/-9 choice index onto a positive dB ceiling.
inline float rangeIndexToDb (int index) noexcept
{
    switch (index)
    {
        case 0:  return 3.0f;
        case 1:  return 6.0f;
        default: return 9.0f;
    }
}

} // namespace zx
