#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "PresetManager.h"
#include "Licensing.h"
#include "DSP/Denoiser.h"
#include "DSP/ToneStack.h"
#include "DSP/DeEsser.h"
#include "DSP/Compressor.h"
#include "DSP/Push.h"
#include "DSP/Saturator.h"
#include "DSP/Limiter.h"

class ZinoxVocalsProcessor : public juce::AudioProcessor
{
public:
    ZinoxVocalsProcessor();
    ~ZinoxVocalsProcessor() override;

    // --- AudioProcessor -----------------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Zinox Vocals"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- editor access ------------------------------------------------------
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    zx::PresetManager& getPresetManager() noexcept { return presetManager; }

    // --- licensing ------------------------------------------------------------
    const zx::License::Info& getLicenseInfo() const noexcept { return licenseInfo; }
    /** Verifies and, if valid, saves the key; updates getLicenseInfo() either way. */
    zx::License::Info activateLicense (const juce::String& licenseBlob);

    /** Metering values, written by the audio thread and read by the UI. */
    struct MeterState
    {
        std::atomic<float> outLeft   { -100.0f };
        std::atomic<float> outRight  { -100.0f };
        std::atomic<float> limitGr   { 0.0f };
        std::atomic<float> deEssGr   { 0.0f };
        std::atomic<float> controlGr { 0.0f };
        std::atomic<float> pushGr    { 0.0f };
    };

    MeterState meters;

private:
    void updateParameters();
    void applyOversampledStage (juce::dsp::AudioBlock<float>& block, double stageSampleRate);
    void updateLatency();

    juce::AudioProcessorValueTreeState apvts;
    zx::PresetManager presetManager;
    zx::License::Info licenseInfo;

    // Unlicensed builds get a brief, unmistakable gain dip every so often
    // instead of silently degrading audio quality - if you hear it, you'll
    // know exactly why. Sample-counted rather than clock-based so it can't
    // be reset by changing the system date.
    juce::int64 samplesUntilNextNag = 0;
    int nagSamplesRemaining = 0;
    int nagTotalSamples = 0;
    void applyUnlicensedNag (juce::AudioBuffer<float>& buffer);

    // --- cached parameter pointers (avoids string lookups per block) --------
    std::atomic<float>* pInput = nullptr;
    std::atomic<float>* pDenoise = nullptr;
    std::atomic<float>* pLowCut = nullptr;
    std::atomic<float>* pHighCut = nullptr;
    std::atomic<float>* pLowGain = nullptr;
    std::atomic<float>* pLowFreq = nullptr;
    std::atomic<float>* pLowShape = nullptr;
    std::atomic<float>* pMidGain = nullptr;
    std::atomic<float>* pMidTone = nullptr;
    std::atomic<float>* pHighGain = nullptr;
    std::atomic<float>* pHighMode = nullptr;
    std::atomic<float>* pDeEss = nullptr;
    std::atomic<float>* pDeEssFreq = nullptr;
    std::atomic<float>* pControl = nullptr;
    std::atomic<float>* pControlRange = nullptr;
    std::atomic<float>* pControlMode = nullptr;
    std::atomic<float>* pPush = nullptr;
    std::atomic<float>* pPushRange = nullptr;
    std::atomic<float>* pPushMode = nullptr;
    std::atomic<float>* pSaturate = nullptr;
    std::atomic<float>* pSatMode = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pLimit = nullptr;
    std::atomic<float>* pLimitOn = nullptr;
    std::atomic<float>* pOversample = nullptr;
    std::atomic<float>* pBypass = nullptr;
    std::atomic<float>* pMix = nullptr;

    // --- dsp ----------------------------------------------------------------
    zx::dsp::Denoiser   denoiser;
    zx::dsp::ToneStack  toneStack;
    zx::dsp::DeEsser    deEsser;
    zx::dsp::Compressor control;
    zx::dsp::Push       push;
    zx::dsp::Saturator  saturator;
    zx::dsp::Limiter    limiter;

    juce::dsp::Gain<float> inputGain, outputGain;
    juce::AudioBuffer<float> dryBuffer;

    // Index 0 = 2x, 1 = 4x, 2 = 8x.  "Off" bypasses them entirely.
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;
    int currentOsChoice = -1;

    double hostSampleRate = 44100.0;
    int    hostBlockSize  = 512;
    int    reportedLatency = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZinoxVocalsProcessor)
};
