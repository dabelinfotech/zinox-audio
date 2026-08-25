#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>

namespace zx::dsp
{

/**
    Real-time adaptive spectral denoiser.

    A short-time Fourier transform continuously learns the noise floor in
    every frequency bin during the quiet parts of the signal — a fast-fall,
    slow-rise tracker that follows the *minimum* envelope of each bin so
    speech energy is never mistaken for noise — then subtracts that floor
    back out with a spectral safety floor, plus smoothing across both
    frequency and time.  The smoothing is what keeps the result free of the
    metallic "musical noise" that raw spectral subtraction is notorious for.

    This is a classical DSP noise reducer, not a trained neural network.  It
    is very effective on steady, stationary noise — hiss, hum, fans, room
    tone, air conditioning — which is the great majority of what shows up
    behind a vocal recording. It will not separate a one-off transient noise
    (a door slam, a keyboard click) from speech the way a model trained
    specifically on that problem can.
*/
class Denoiser
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        fs = spec.sampleRate;

        // Keep the analysis window a fixed ~21 ms regardless of sample rate,
        // rounded up to the nearest power of two the FFT requires.
        int size = 512;
        while (size < (int) std::round (fs * 0.0213))
            size <<= 1;
        fftSize = juce::jlimit (512, 8192, size);
        hopSize = fftSize / 2;
        numBins = fftSize / 2 + 1;

        fftOrder = (int) std::round (std::log2 ((double) fftSize));
        fft = std::make_unique<juce::dsp::FFT> (fftOrder);

        // A Hann analysis window with no separate synthesis window keeps the
        // overlap-add exact at 50% hop: when every bin's gain is 1, summing
        // the overlapped windowed frames reconstructs the input bit-for-bit
        // (modulo floating point), so turning the effect on and off never
        // clicks.
        window.assign ((size_t) fftSize, 0.0f);
        for (int n = 0; n < fftSize; ++n)
            window[(size_t) n] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) n
                                                          / (float) (fftSize - 1));

        // Gain ballistics are expressed against the hop rate, since gain is
        // only updated once per frame, not once per sample.
        const auto hopSeconds = (float) hopSize / (float) fs;
        gainAttackCoeff  = std::exp (-hopSeconds / 0.008f);   // ~8 ms  - clamp down quickly
        gainReleaseCoeff = std::exp (-hopSeconds / 0.080f);   // ~80 ms - let the voice back in gently

        // The noise floor rides out short pauses in speech without being
        // fooled by them, but still follows a real, slow change in the room
        // (an AC unit cycling on) over a few seconds.
        noiseRiseCoeff = std::exp (-hopSeconds / 4.0f);

        channels.clear();
        channels.resize ((size_t) juce::jmax (1u, spec.numChannels));

        for (auto& ch : channels)
        {
            ch.fifo.assign ((size_t) fftSize, 0.0f);
            ch.outputAccum.assign ((size_t) fftSize, 0.0f);
            ch.noiseMagnitude.assign ((size_t) numBins, 0.0f);
            ch.gain.assign ((size_t) numBins, 1.0f);
            ch.fifoIndex = 0;
            ch.samplesUntilHop = hopSize;
        }

        fftWorkspace.assign ((size_t) fftSize * 2, 0.0f);
        windowed.assign ((size_t) fftSize, 0.0f);
        rawGain.assign ((size_t) numBins, 1.0f);
    }

    void reset()
    {
        for (auto& ch : channels)
        {
            std::fill (ch.fifo.begin(), ch.fifo.end(), 0.0f);
            std::fill (ch.outputAccum.begin(), ch.outputAccum.end(), 0.0f);
            std::fill (ch.noiseMagnitude.begin(), ch.noiseMagnitude.end(), 0.0f);
            std::fill (ch.gain.begin(), ch.gain.end(), 1.0f);
            ch.fifoIndex = 0;
            ch.samplesUntilHop = hopSize;
        }
    }

    void setAmount (float amount01) noexcept { amount = juce::jlimit (0.0f, 1.0f, amount01); }

    /** Constant regardless of amount, so turning the knob never re-negotiates
        host latency or introduces a click. */
    int getLatencySamples() const noexcept { return fftSize - hopSize; }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const auto numCh = juce::jmin (buffer.getNumChannels(), (int) channels.size());
        const auto numSamples = buffer.getNumSamples();

        for (int ch = 0; ch < numCh; ++ch)
            processChannel (channels[(size_t) ch], buffer.getWritePointer (ch), numSamples);
    }

private:
    struct ChannelState
    {
        std::vector<float> fifo;           // fftSize - ring buffer of input history
        std::vector<float> outputAccum;    // fftSize - ring buffer being overlap-added into
        std::vector<float> noiseMagnitude; // numBins - tracked noise floor per bin
        std::vector<float> gain;           // numBins - smoothed gain carried from the previous frame
        int fifoIndex = 0;
        int samplesUntilHop = 0;
    };

    void processChannel (ChannelState& state, float* data, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto pos = state.fifoIndex;
            const auto inSample = data[i];

            // Input and output share one ring position: emit whatever this
            // slot finished accumulating from earlier frames (the delayed,
            // processed signal), clear it, then store the new input sample
            // for future frames to read.
            const auto outSample = state.outputAccum[(size_t) pos];
            state.outputAccum[(size_t) pos] = 0.0f;
            state.fifo[(size_t) pos] = inSample;

            data[i] = outSample;

            state.fifoIndex = (pos + 1) % fftSize;

            if (--state.samplesUntilHop == 0)
            {
                state.samplesUntilHop = hopSize;
                processFrame (state);
            }
        }
    }

    void processFrame (ChannelState& state)
    {
        // fifoIndex now points at the oldest sample of the window that just
        // completed, since it was advanced one past the newest sample.
        const auto start = state.fifoIndex;

        for (int n = 0; n < fftSize; ++n)
            windowed[(size_t) n] = state.fifo[(size_t) ((start + n) % fftSize)] * window[(size_t) n];

        std::fill (fftWorkspace.begin(), fftWorkspace.end(), 0.0f);
        std::copy (windowed.begin(), windowed.end(), fftWorkspace.begin());

        fft->performRealOnlyForwardTransform (fftWorkspace.data(), false);

        const bool activeAmount = amount > 0.0005f;
        const auto alpha = juce::jmap (amount, 0.0f, 1.0f, 1.0f, 6.0f);   // oversubtraction
        const auto beta  = juce::jmap (amount, 0.0f, 1.0f, 0.35f, 0.02f); // spectral floor

        // --- pass 1: magnitude, noise-floor tracking, raw subtraction gain --
        for (int b = 0; b < numBins; ++b)
        {
            const auto re = fftWorkspace[(size_t) (2 * b)];
            const auto im = fftWorkspace[(size_t) (2 * b + 1)];
            const auto mag = std::sqrt (re * re + im * im);

            auto& noiseMag = state.noiseMagnitude[(size_t) b];
            noiseMag = mag < noiseMag ? mag : noiseRiseCoeff * noiseMag + (1.0f - noiseRiseCoeff) * mag;

            if (! activeAmount || mag < 1.0e-9f)
                rawGain[(size_t) b] = 1.0f;
            else
                rawGain[(size_t) b] = juce::jlimit (beta, 1.0f, (mag - alpha * noiseMag) / mag);
        }

        // --- pass 2: smooth across frequency and time, then apply -----------
        for (int b = 0; b < numBins; ++b)
        {
            const auto lo = rawGain[(size_t) juce::jmax (0, b - 1)];
            const auto hi = rawGain[(size_t) juce::jmin (numBins - 1, b + 1)];
            const auto targetGain = 0.25f * lo + 0.5f * rawGain[(size_t) b] + 0.25f * hi;

            auto& g = state.gain[(size_t) b];
            const auto coeff = targetGain < g ? gainAttackCoeff : gainReleaseCoeff;
            g = coeff * g + (1.0f - coeff) * targetGain;

            fftWorkspace[(size_t) (2 * b)]     *= g;
            fftWorkspace[(size_t) (2 * b + 1)] *= g;
        }

        // Mirror the modified half back into the redundant upper half so
        // every FFT backend gets a fully Hermitian-symmetric spectrum to
        // invert, guaranteeing a purely real time-domain result.
        for (int b = 1; b < numBins - 1; ++b)
        {
            fftWorkspace[(size_t) (2 * (fftSize - b))]     =  fftWorkspace[(size_t) (2 * b)];
            fftWorkspace[(size_t) (2 * (fftSize - b) + 1)] = -fftWorkspace[(size_t) (2 * b + 1)];
        }

        fft->performRealOnlyInverseTransform (fftWorkspace.data());

        for (int n = 0; n < fftSize; ++n)
            state.outputAccum[(size_t) ((start + n) % fftSize)] += fftWorkspace[(size_t) n];
    }

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window;
    std::vector<ChannelState> channels;
    std::vector<float> fftWorkspace, windowed, rawGain;

    int fftSize = 1024, hopSize = 512, numBins = 513, fftOrder = 10;
    double fs = 44100.0;
    float  amount = 0.0f;
    float  gainAttackCoeff = 0.0f, gainReleaseCoeff = 0.0f, noiseRiseCoeff = 0.0f;
};

} // namespace zx::dsp
