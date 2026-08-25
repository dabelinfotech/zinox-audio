#pragma once

#include "Envelope.h"
#include <juce_dsp/juce_dsp.h>

namespace zx::dsp
{

/**
    Four-flavour saturation stage.  Runs inside the oversampled section so the
    harmonics it generates fold back as little as possible.

    RICH  - asymmetric tanh, strong 2nd + 3rd harmonic, slight presence lift
    WARM  - soft cubic clip with low-mid emphasis, rounds the top end
    TAPE  - tanh plus a frequency-dependent squash and gentle HF loss
    TUBE  - heavily asymmetric, dominant even harmonics, bass tightening
*/
class Saturator
{
public:
    enum class Mode { Rich = 0, Warm, Tape, Tube };

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        fs = spec.sampleRate;

        preEmphasis.prepare (spec);
        postEmphasis.prepare (spec);
        dcBlocker.prepare (spec);
        tapeLoss.prepare (spec);

        drive.prepare (spec.sampleRate, 20.0f);
        drive.snap (1.0f);
        trim.prepare (spec.sampleRate, 20.0f);
        trim.snap (1.0f);

        reset();
        updateFilters();
    }

    void setSampleRate (double sampleRate) noexcept
    {
        if (! juce::approximatelyEqual (fs, sampleRate))
        {
            fs = sampleRate;
            updateFilters();
        }
    }

    void reset()
    {
        preEmphasis.reset();
        postEmphasis.reset();
        dcBlocker.reset();
        tapeLoss.reset();
    }

    /** @param amount 0..1 front-panel knob. */
    void setParams (float amount, Mode m)
    {
        active = amount > 0.0005f;

        if (m != mode)
        {
            mode = m;
            updateFilters();
        }

        // Drive rises to ~+22 dB at full; output trim compensates so the knob
        // changes character rather than loudness.
        const auto driveDb = juce::jmap (amount, 0.0f, 1.0f, 0.0f, 22.0f);
        drive.setTarget (dbToGain (driveDb));
        trim.setTarget (dbToGain (-driveDb * compensationFactor()));
        amountLin = amount;
    }

    template <typename BlockType>
    void process (BlockType& block) noexcept
    {
        if (! active)
            return;

        const auto numCh = (int) block.getNumChannels();
        const auto numSamples = (int) block.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            const auto d = drive.next();
            const auto t = trim.next();

            for (int ch = 0; ch < numCh; ++ch)
            {
                auto x = block.getSample (ch, i);

                x = preEmphasis.processSample (ch, x);
                x = shape (x * d);
                x = postEmphasis.processSample (ch, x);
                x = dcBlocker.processSample (ch, x);

                if (mode == Mode::Tape)
                    x = tapeLoss.processSample (ch, x);

                block.setSample (ch, i, x * t);
            }
        }

        preEmphasis.snapToZero();
        postEmphasis.snapToZero();
        dcBlocker.snapToZero();
        tapeLoss.snapToZero();
    }

private:
    float shape (float x) noexcept
    {
        switch (mode)
        {
            case Mode::Rich:
            {
                // Asymmetric tanh: the bias term is what generates the even
                // harmonics that make vocals sit forward in a mix.
                const auto bias = 0.18f * amountLin;
                return std::tanh (x + bias) - std::tanh (bias);
            }

            case Mode::Warm:
            {
                const auto c = juce::jlimit (-1.5f, 1.5f, x * 0.66f);
                return 1.5f * (c - c * c * c / 3.0f);
            }

            case Mode::Tape:
            {
                // Level-dependent squash: louder passages compress harder,
                // which is the part of tape that people actually hear.
                const auto squash = 1.0f / (1.0f + 0.25f * amountLin * std::abs (x));
                return std::tanh (x * squash) * (1.0f + 0.12f * amountLin);
            }

            case Mode::Tube:
            default:
            {
                // Different curve above and below zero — classic single-ended
                // triode asymmetry.
                const auto k = 1.0f + 2.5f * amountLin;
                if (x >= 0.0f)
                    return (1.0f - std::exp (-x * k)) / k * 1.6f;

                const auto neg = -x * k * 0.7f;
                return -(1.0f - std::exp (-neg)) / (k * 0.7f) * 1.6f;
            }
        }
    }

    float compensationFactor() const noexcept
    {
        switch (mode)
        {
            case Mode::Rich: return 0.62f;
            case Mode::Warm: return 0.55f;
            case Mode::Tape: return 0.70f;
            case Mode::Tube:
            default:         return 0.66f;
        }
    }

    void updateFilters()
    {
        using Coeffs = juce::dsp::IIR::Coefficients<float>;

        // Pre/post emphasis pairs decide *where* in the spectrum the distortion
        // lands.  They are exact inverses so the tonal balance stays put.
        float freq = 1200.0f, gainDb = 0.0f;

        switch (mode)
        {
            case Mode::Rich: freq = 3000.0f; gainDb =  3.5f; break;
            case Mode::Warm: freq =  450.0f; gainDb =  4.5f; break;
            case Mode::Tape: freq =  900.0f; gainDb =  2.5f; break;
            case Mode::Tube: freq =  180.0f; gainDb = -3.0f; break;
        }

        preEmphasis .setCoefficients (Coeffs::makePeakFilter (fs, freq, 0.8f, dbToGain (gainDb)));
        postEmphasis.setCoefficients (Coeffs::makePeakFilter (fs, freq, 0.8f, dbToGain (-gainDb)));

        dcBlocker.setCoefficients (Coeffs::makeHighPass (fs, 18.0f, 0.7071f));
        tapeLoss .setCoefficients (Coeffs::makeFirstOrderLowPass (fs, juce::jmin (17000.0f, (float) fs * 0.42f)));
    }

    // Per-channel filter pair, so the nonlinearity can run sample-by-sample.
    struct DualFilter
    {
        void prepare (const juce::dsp::ProcessSpec& spec)
        {
            for (auto& f : filters)
                f.prepare ({ spec.sampleRate, spec.maximumBlockSize, 1 });
        }
        void reset()      { for (auto& f : filters) f.reset(); }
        void snapToZero() { for (auto& f : filters) f.snapToZero(); }

        void setCoefficients (juce::dsp::IIR::Coefficients<float>::Ptr c)
        {
            for (auto& f : filters)
                f.coefficients = c;
        }

        float processSample (int ch, float x) noexcept
        {
            return filters[(size_t) juce::jlimit (0, 1, ch)].processSample (x);
        }

        std::array<juce::dsp::IIR::Filter<float>, 2> filters;
    };

    DualFilter preEmphasis, postEmphasis, dcBlocker, tapeLoss;
    Smoothed drive, trim;

    double fs = 44100.0;
    Mode   mode = Mode::Rich;
    float  amountLin = 0.0f;
    bool   active = false;
};

} // namespace zx::dsp
