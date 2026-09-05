#pragma once

#include "Envelope.h"
#include <juce_dsp/juce_dsp.h>

namespace zx::dsp
{

/**
    The fixed-topology vocal EQ: low cut, high cut, and three broad musical
    bands.  Every band uses a wide Q so the controls behave like a console
    channel rather than a surgical equaliser — you can push them hard and the
    result still sounds like the singer.
*/
class ToneStack
{
public:
    enum class LowShape  { Peak = 0, Shelf };
    enum class HighMode  { Air = 0, Bright, Presence };

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        fs = spec.sampleRate;

        for (auto* f : allFilters())
            f->prepare (spec);

        update (true);
    }

    void reset()
    {
        for (auto* f : allFilters())
            f->reset();
    }

    void setParams (float lowCutHz, float highCutHz,
                    float lowGainDb, float lowFreqHz, LowShape lowShapeIn,
                    float midGainDb, float midToneNorm,
                    float highGainDb, HighMode highModeIn)
    {
        // Building IIR coefficients allocates, so only do it when something
        // actually moved — setParams is called once per block.
        const bool unchanged =
               juce::approximatelyEqual (lowCutHz,   lowCut)
            && juce::approximatelyEqual (highCutHz,  highCut)
            && juce::approximatelyEqual (lowGainDb,  lowGain)
            && juce::approximatelyEqual (lowFreqHz,  lowFreq)
            && juce::approximatelyEqual (midGainDb,  midGain)
            && juce::approximatelyEqual (midToneNorm, midTone)
            && juce::approximatelyEqual (highGainDb, highGain)
            && lowShapeIn == lowShape
            && highModeIn == highMode;

        if (unchanged)
            return;

        lowCut    = lowCutHz;
        highCut   = highCutHz;
        lowGain   = lowGainDb;
        lowFreq   = lowFreqHz;
        lowShape  = lowShapeIn;
        midGain   = midGainDb;
        midTone   = midToneNorm;
        highGain  = highGainDb;
        highMode  = highModeIn;

        lowCutActive  = lowCutHz  > 20.5f;
        highCutActive = highCutHz < 21500.0f;

        update (false);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);

        if (lowCutActive)
        {
            hpf1.process (ctx);
            hpf2.process (ctx);
        }

        if (highCutActive)
        {
            lpf1.process (ctx);
            lpf2.process (ctx);
        }

        if (std::abs (lowGain)  > 0.01f) lowBand .process (ctx);
        if (std::abs (midGain)  > 0.01f) midBand .process (ctx);
        if (std::abs (highGain) > 0.01f) highBand.process (ctx);
    }

private:
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, Coeffs>;

    std::array<Filter*, 7> allFilters()
    {
        return { &hpf1, &hpf2, &lpf1, &lpf2, &lowBand, &midBand, &highBand };
    }

    void update (bool force)
    {
        juce::ignoreUnused (force);

        const auto nyquist = (float) fs * 0.5f;
        const auto clampF  = [nyquist] (float f) { return juce::jlimit (20.0f, nyquist * 0.95f, f); };

        // 12 dB/oct cascaded to 24 dB/oct — steep enough to actually clear
        // rumble without needing to be dialled up into the voice.
        *hpf1.state = *Coeffs::makeHighPass (fs, clampF (lowCut), 0.5412f);
        *hpf2.state = *Coeffs::makeHighPass (fs, clampF (lowCut), 1.3066f);

        *lpf1.state = *Coeffs::makeLowPass (fs, clampF (highCut), 0.5412f);
        *lpf2.state = *Coeffs::makeLowPass (fs, clampF (highCut), 1.3066f);

        if (lowShape == LowShape::Shelf)
            *lowBand.state = *Coeffs::makeLowShelf (fs, clampF (lowFreq), 0.72f, dbToGain (lowGain));
        else
            *lowBand.state = *Coeffs::makePeakFilter (fs, clampF (lowFreq), 0.9f, dbToGain (lowGain));

        // TONE sweeps the mid centre from body (350 Hz) to bite (5 kHz).
        const auto midFreq = 350.0f * std::pow (5000.0f / 350.0f, midTone);
        *midBand.state = *Coeffs::makePeakFilter (fs, clampF (midFreq), 0.75f, dbToGain (midGain));

        // The High band reads 40% sharper than its dB display: each dB the
        // knob moves drives 1.4x the gain into the filter, so the control
        // still shows ±18 dB but cuts and boosts noticeably harder than a
        // literal reading of that number would suggest.
        constexpr float highGainSharpness = 1.4f;
        const auto highGainApplied = highGain * highGainSharpness;

        switch (highMode)
        {
            case HighMode::Air:
                *highBand.state = *Coeffs::makeHighShelf (fs, clampF (11000.0f), 0.5f, dbToGain (highGainApplied));
                break;
            case HighMode::Bright:
                *highBand.state = *Coeffs::makeHighShelf (fs, clampF (6500.0f), 0.62f, dbToGain (highGainApplied));
                break;
            case HighMode::Presence:
            default:
                *highBand.state = *Coeffs::makePeakFilter (fs, clampF (3800.0f), 0.8f, dbToGain (highGainApplied));
                break;
        }
    }

    Filter hpf1, hpf2, lpf1, lpf2, lowBand, midBand, highBand;

    double fs = 44100.0;
    float  lowCut = 20.0f, highCut = 22000.0f;
    float  lowGain = 0.0f, lowFreq = 100.0f;
    float  midGain = 0.0f, midTone = 0.5f;
    float  highGain = 0.0f;
    LowShape lowShape = LowShape::Shelf;
    HighMode highMode = HighMode::Air;
    bool   lowCutActive = false, highCutActive = false;
};

} // namespace zx::dsp
