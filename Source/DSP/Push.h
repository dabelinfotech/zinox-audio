#pragma once

#include "Compressor.h"
#include "Envelope.h"
#include <juce_dsp/juce_dsp.h>

namespace zx::dsp
{

/**
    "Push" — the density stage.

    A hard, fast compressor with drive running in parallel with the dry signal.
    Blending rather than replacing is what lets it raise the floor of a vocal
    without flattening the transients the main Control compressor is shaping.

    PUNCH  slow attack, fast release, keeps consonants intact
    FAT    fast attack, slow release, maximum sustain
    TIGHT  very fast both ways, aggressive and controlled
*/
class Push
{
public:
    enum class Mode { Punch = 0, Fat, Tight };

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        fs = spec.sampleRate;
        comp.prepare (spec.sampleRate, (int) spec.numChannels);
        wetBuffer.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);
        blend.prepare (spec.sampleRate, 30.0f);
        blend.snap (0.0f);
        reset();
    }

    void setSampleRate (double sampleRate) noexcept
    {
        fs = sampleRate;
        comp.setSampleRate (sampleRate);
        blend.prepare (sampleRate, 30.0f);
        pushSettings();
    }

    void reset()
    {
        comp.reset();
        wetBuffer.clear();
        currentGr = 0.0f;
    }

    void setParams (float amountIn, Mode m, float rangeDbIn)
    {
        active  = amountIn > 0.0005f;
        amount  = amountIn;
        mode    = m;
        rangeDb = rangeDbIn;
        blend.setTarget (amount);
        pushSettings();
    }

    template <typename BlockType>
    void process (BlockType& block)
    {
        if (! active)
        {
            currentGr = 0.0f;
            return;
        }

        const auto numCh = (int) block.getNumChannels();
        const auto numSamples = (int) block.getNumSamples();

        wetBuffer.setSize (numCh, numSamples, false, false, true);

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* dst = wetBuffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                dst[i] = block.getSample (ch, i) * driveGain;
        }

        juce::dsp::AudioBlock<float> wet (wetBuffer);
        auto sub = wet.getSubBlock (0, (size_t) numSamples);
        comp.process (sub);
        currentGr = comp.getGainReductionDb();

        for (int i = 0; i < numSamples; ++i)
        {
            const auto b = blend.next();

            for (int ch = 0; ch < numCh; ++ch)
            {
                const auto dry = block.getSample (ch, i);
                auto w = wetBuffer.getSample (ch, i) * makeupGain;

                // Gentle clip on the wet path only — keeps the parallel branch
                // from ever being the thing that overshoots.
                w = std::tanh (w * 1.15f) * 0.87f;

                block.setSample (ch, i, dry * (1.0f - b * 0.35f) + w * b);
            }
        }
    }

    float getGainReductionDb() const noexcept { return currentGr; }

private:
    void pushSettings()
    {
        Compressor::Settings s;

        switch (mode)
        {
            case Mode::Punch: s.attackMs = 18.0f; s.releaseMs =  90.0f; s.ratio = 6.0f;  s.kneeDb = 8.0f; break;
            case Mode::Fat:   s.attackMs =  2.0f; s.releaseMs = 260.0f; s.ratio = 8.0f;  s.kneeDb = 6.0f; break;
            case Mode::Tight: s.attackMs =  0.4f; s.releaseMs =  45.0f; s.ratio = 12.0f; s.kneeDb = 3.0f; break;
        }

        s.thresholdDb = juce::jmap (amount, 0.0f, 1.0f, -8.0f, -34.0f);
        s.maxGrDb     = rangeDb;
        s.makeupDb    = 0.0f;
        s.rmsDetect   = (mode == Mode::Fat);

        comp.setSettings (s);

        driveGain  = dbToGain (juce::jmap (amount, 0.0f, 1.0f, 0.0f, 9.0f));
        makeupGain = dbToGain (rangeDb * 0.55f) / driveGain;
    }

    Compressor comp;
    juce::AudioBuffer<float> wetBuffer;
    Smoothed blend;

    double fs = 44100.0;
    Mode   mode = Mode::Punch;
    float  amount = 0.0f, rangeDb = 6.0f;
    float  driveGain = 1.0f, makeupGain = 1.0f, currentGr = 0.0f;
    bool   active = false;
};

} // namespace zx::dsp
