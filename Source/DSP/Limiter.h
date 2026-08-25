#pragma once

#include "Envelope.h"
#include <juce_dsp/juce_dsp.h>

namespace zx::dsp
{

/**
    Look-ahead brickwall limiter.

    A short delay line holds the audio while the detector looks at what is
    coming, so the gain is already down by the time the peak arrives.  The gain
    signal itself is smoothed twice — once by a max-hold window and once by a
    one-pole release — which removes the ripple that a single-stage limiter
    puts on low-frequency material.
*/
class Limiter
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        fs = spec.sampleRate;
        numChannels = (int) spec.numChannels;

        lookaheadSamples = juce::jmax (1, (int) std::round (lookaheadMs * 0.001 * fs));

        // Ring buffer is exactly one sample longer than the look-ahead, so
        // reading at (writePos + 1) yields a delay of `lookaheadSamples`.
        delayLine.setSize (numChannels, lookaheadSamples + 1);
        delayLine.clear();
        writePos = 0;

        holdBuffer.assign ((size_t) lookaheadSamples + 1, 0.0f);
        holdPos = 0;

        release.setTimes (fs, 0.0f, 60.0f);
        release.reset (0.0f);

        smoother.prepare (fs, 1.2f);
        smoother.snap (1.0f);

        currentGr = 0.0f;
    }

    void reset()
    {
        delayLine.clear();
        writePos = 0;
        std::fill (holdBuffer.begin(), holdBuffer.end(), 0.0f);
        holdPos = 0;
        release.reset (0.0f);
        smoother.snap (1.0f);
        currentGr = 0.0f;
    }

    void setCeiling (float ceilingDb) noexcept
    {
        ceiling = dbToGain (ceilingDb);
        ceilingInDb = ceilingDb;
    }

    void setEnabled (bool shouldBeOn) noexcept { enabled = shouldBeOn; }

    /** Constant regardless of the enabled state: the look-ahead delay is always
        in circuit, so bypassing the limiter never re-negotiates host latency
        or jumps the audio forward by 1.5 ms. */
    int getLatencySamples() const noexcept { return lookaheadSamples; }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const auto numCh = juce::jmin (buffer.getNumChannels(), numChannels);
        const auto numSamples = buffer.getNumSamples();
        const auto delaySize = delayLine.getNumSamples();

        float peakGr = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // --- push the incoming sample into the look-ahead delay ---------
            float inPeak = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
            {
                const auto x = buffer.getSample (ch, i);
                delayLine.setSample (ch, writePos, x);
                inPeak = std::max (inPeak, std::abs (x));
            }

            if (! enabled)
            {
                const auto readPos = (writePos + 1) % delaySize;
                for (int ch = 0; ch < numCh; ++ch)
                    buffer.setSample (ch, i, delayLine.getSample (ch, readPos));
                writePos = (writePos + 1) % delaySize;
                continue;
            }

            // --- required gain for this incoming peak -----------------------
            const auto required = inPeak > ceiling ? ceiling / inPeak : 1.0f;

            // --- max-hold over the look-ahead window ------------------------
            holdBuffer[(size_t) holdPos] = required;
            holdPos = (holdPos + 1) % (int) holdBuffer.size();

            float minGain = 1.0f;
            for (auto g : holdBuffer)
                minGain = std::min (minGain, g);

            // --- release smoothing (attack is instantaneous) ----------------
            const auto targetDb = gainToDb (minGain);
            const auto smoothedDb = -release.processPeak (-targetDb);
            smoother.setTarget (dbToGain (smoothedDb));
            const auto g = smoother.next();

            peakGr = std::max (peakGr, -gainToDb (g));

            // --- read the delayed sample and apply --------------------------
            const auto readPos = (writePos + 1) % delaySize;
            for (int ch = 0; ch < numCh; ++ch)
            {
                auto out = delayLine.getSample (ch, readPos) * g;
                out = juce::jlimit (-ceiling, ceiling, out);   // absolute safety net
                buffer.setSample (ch, i, out);
            }

            writePos = (writePos + 1) % delaySize;
        }

        currentGr = peakGr;
    }

    float getGainReductionDb() const noexcept { return currentGr; }
    float getCeilingDb()       const noexcept { return ceilingInDb; }

private:
    juce::AudioBuffer<float> delayLine;
    std::vector<float> holdBuffer;
    Ballistics release;
    Smoothed   smoother;

    double fs = 44100.0;
    double lookaheadMs = 1.5;
    int    lookaheadSamples = 64, writePos = 0, holdPos = 0, numChannels = 2;
    float  ceiling = 1.0f, ceilingInDb = 0.0f, currentGr = 0.0f;
    bool   enabled = true;
};

} // namespace zx::dsp
