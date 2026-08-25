#pragma once

#include "Envelope.h"
#include <juce_dsp/juce_dsp.h>

namespace zx::dsp
{

/**
    Stereo-linked feed-forward compressor with a soft knee and a hard ceiling on
    how much gain reduction it is allowed to apply.

    The ceiling is what makes the "-3 / -6 / -9" switches on the front panel
    behave the way they do on a modern vocal strip: you turn the amount knob up
    and the detector works harder, but the gain reduction can never exceed the
    selected range, so the compressor stays musical instead of collapsing.
*/
class Compressor
{
public:
    struct Settings
    {
        float thresholdDb = 0.0f;
        float ratio       = 4.0f;
        float kneeDb      = 6.0f;
        float attackMs    = 10.0f;
        float releaseMs   = 120.0f;
        float maxGrDb     = 6.0f;    // hard ceiling on gain reduction
        float makeupDb    = 0.0f;
        bool  rmsDetect   = false;
    };

    void prepare (double sampleRate, int numChannels)
    {
        fs = sampleRate;
        channels = numChannels;
        gr.reset (0.0f);
        rms.reset (0.0f);
        makeup.prepare (sampleRate, 20.0f);
        makeup.snap (1.0f);
        currentGrDb = 0.0f;
    }

    void setSampleRate (double sampleRate) noexcept { fs = sampleRate; }

    void setSettings (const Settings& s) noexcept
    {
        settings = s;
        gr.setTimes (fs, s.attackMs, s.releaseMs);
        rms.setTimes (fs, 5.0f, 5.0f);
        makeup.setTarget (dbToGain (s.makeupDb));
    }

    void reset() noexcept
    {
        gr.reset (0.0f);
        rms.reset (0.0f);
        currentGrDb = 0.0f;
    }

    /** Processes an interleaved-by-channel block in place, stereo linked. */
    template <typename BlockType>
    void process (BlockType& block) noexcept
    {
        const auto numCh = (int) block.getNumChannels();
        const auto numSamples = (int) block.getNumSamples();

        float peakGr = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // --- stereo-linked detector -------------------------------------
            float detect = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
                detect = std::max (detect, std::abs (block.getSample (ch, i)));

            if (settings.rmsDetect)
                detect = std::sqrt (rms.process (detect * detect));

            const auto inDb = gainToDb (detect);

            // --- soft-knee static curve -------------------------------------
            const auto over = inDb - settings.thresholdDb;
            float reduction = 0.0f;

            if (settings.kneeDb > 0.0f && over > -settings.kneeDb * 0.5f && over < settings.kneeDb * 0.5f)
            {
                const auto x = over + settings.kneeDb * 0.5f;
                reduction = (1.0f / settings.ratio - 1.0f) * x * x / (2.0f * settings.kneeDb);
            }
            else if (over >= settings.kneeDb * 0.5f)
            {
                reduction = (1.0f / settings.ratio - 1.0f) * over;
            }

            // reduction is <= 0.  Clamp to the selected range.
            reduction = std::max (reduction, -settings.maxGrDb);

            // --- ballistics on the gain-reduction signal --------------------
            const auto smoothedGr = gr.process (reduction);
            peakGr = std::max (peakGr, -smoothedGr);

            const auto g = dbToGain (smoothedGr) * makeup.next();

            for (int ch = 0; ch < numCh; ++ch)
                block.setSample (ch, i, block.getSample (ch, i) * g);
        }

        currentGrDb = peakGr;
    }

    float getGainReductionDb() const noexcept { return currentGrDb; }

private:
    Settings  settings;
    Ballistics gr, rms;
    Smoothed  makeup;
    double    fs = 44100.0;
    int       channels = 2;
    float     currentGrDb = 0.0f;
};

} // namespace zx::dsp
