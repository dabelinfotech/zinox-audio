#pragma once

#include "Envelope.h"
#include <juce_dsp/juce_dsp.h>

namespace zx::dsp
{

/**
    Split-band de-esser.

    The signal is divided by a Linkwitz-Riley crossover, the high band is
    compressed on its own emphasised detector, and the two bands are summed
    again.  Because the crossover is phase-coherent the recombination is
    transparent whenever the de-esser is idle.
*/
class DeEsser
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        fs = spec.sampleRate;

        crossover.prepare (spec);
        crossover.setCutoffFrequency (6500.0f);

        // Force setParams to rebuild the detector coefficients for this rate.
        lastFreq = -1.0f;

        env.reset (0.0f);
        env.setTimes (fs, 0.5f, 45.0f);

        for (auto& f : detectFilters)
            f.reset();

        currentGr = 0.0f;
    }

    void reset()
    {
        crossover.reset();
        env.reset (0.0f);
        for (auto& f : detectFilters)
            f.reset();
        currentGr = 0.0f;
    }

    /** @param amount 0..1 front-panel knob.  @param freq crossover in Hz. */
    void setParams (float amount, float freq) noexcept
    {
        active = amount > 0.0005f;

        const auto f = juce::jlimit (1000.0f, 16000.0f, freq);

        // The amount knob drives threshold down and ratio up together, so a
        // single control moves from "gentle polish" to "hard sibilance tamer".
        thresholdDb = juce::jmap (amount, 0.0f, 1.0f, -6.0f, -42.0f);
        ratio       = juce::jmap (amount, 0.0f, 1.0f, 2.0f, 10.0f);
        maxGrDb     = juce::jmap (amount, 0.0f, 1.0f, 3.0f, 24.0f);

        // Coefficient construction allocates — skip it unless the frequency
        // really moved, since this runs once per block.
        if (juce::approximatelyEqual (f, lastFreq))
            return;

        lastFreq = f;
        crossover.setCutoffFrequency (f);

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (fs, f, 0.7f, 2.0f);
        for (auto& filt : detectFilters)
            filt.coefficients = coeffs;
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        if (! active)
        {
            currentGr = 0.0f;
            return;
        }

        const auto numCh = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();

        float peakGr = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // Split every channel first so the detector can be stereo linked.
            float lo[2] { 0.0f, 0.0f }, hi[2] { 0.0f, 0.0f };
            float detect = 0.0f;

            for (int ch = 0; ch < juce::jmin (numCh, 2); ++ch)
            {
                crossover.processSample (ch, buffer.getSample (ch, i), lo[ch], hi[ch]);

                const auto emphasised = detectFilters[(size_t) ch].processSample (hi[ch]);
                detect = std::max (detect, std::abs (emphasised));
            }

            const auto over = gainToDb (detect) - thresholdDb;

            float reduction = over > 0.0f ? (1.0f / ratio - 1.0f) * over : 0.0f;
            reduction = std::max (reduction, -maxGrDb);

            const auto smoothed = env.process (reduction);
            peakGr = std::max (peakGr, -smoothed);

            const auto g = dbToGain (smoothed);

            for (int ch = 0; ch < juce::jmin (numCh, 2); ++ch)
                buffer.setSample (ch, i, lo[ch] + hi[ch] * g);
        }

        currentGr = peakGr;
    }

    float getGainReductionDb() const noexcept { return currentGr; }

private:
    juce::dsp::LinkwitzRileyFilter<float> crossover;
    std::array<juce::dsp::IIR::Filter<float>, 2> detectFilters;
    Ballistics env;

    double fs = 44100.0;
    float  thresholdDb = -20.0f, ratio = 4.0f, maxGrDb = 12.0f, currentGr = 0.0f;
    float  lastFreq = -1.0f;
    bool   active = false;
};

} // namespace zx::dsp
