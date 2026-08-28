#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace zx;

ZinoxVocalsProcessor::ZinoxVocalsProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ZINOX_VOCALS", createParameterLayout()),
      presetManager (apvts),
      licenseInfo (zx::License::loadSaved()),
      trialStatus (zx::Trial::checkAndUpdate())
{
    updateTrialBlockedFlag();

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id); };

    pInput        = get (ParamID::input);
    pDenoise      = get (ParamID::denoise);
    pLowCut       = get (ParamID::lowCut);
    pHighCut      = get (ParamID::highCut);
    pLowGain      = get (ParamID::lowGain);
    pLowFreq      = get (ParamID::lowFreq);
    pLowShape     = get (ParamID::lowShape);
    pMidGain      = get (ParamID::midGain);
    pMidTone      = get (ParamID::midTone);
    pHighGain     = get (ParamID::highGain);
    pHighMode     = get (ParamID::highMode);
    pDeEss        = get (ParamID::deEss);
    pDeEssFreq    = get (ParamID::deEssFreq);
    pControl      = get (ParamID::control);
    pControlRange = get (ParamID::controlRange);
    pControlMode  = get (ParamID::controlMode);
    pPush         = get (ParamID::push);
    pPushRange    = get (ParamID::pushRange);
    pPushMode     = get (ParamID::pushMode);
    pSaturate     = get (ParamID::saturate);
    pSatMode      = get (ParamID::satMode);
    pOutput       = get (ParamID::output);
    pLimit        = get (ParamID::limit);
    pLimitOn      = get (ParamID::limitOn);
    pOversample   = get (ParamID::oversample);
    pBypass       = get (ParamID::bypass);
    pMix          = get (ParamID::mix);
}

ZinoxVocalsProcessor::~ZinoxVocalsProcessor() = default;

// ---------------------------------------------------------------------------

void ZinoxVocalsProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostSampleRate = sampleRate;
    hostBlockSize  = samplesPerBlock;

    const auto numCh = (juce::uint32) juce::jmax (1, getTotalNumOutputChannels());

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, numCh };

    denoiser.prepare (spec);
    toneStack.prepare (spec);
    deEsser.prepare (spec);
    control.prepare (sampleRate, (int) numCh);
    limiter.prepare (spec);

    inputGain.prepare (spec);
    outputGain.prepare (spec);
    inputGain.setRampDurationSeconds (0.02);
    outputGain.setRampDurationSeconds (0.02);

    dryBuffer.setSize ((int) numCh, samplesPerBlock);
    dryBuffer.clear();

    // Oversamplers for 2x / 4x / 8x.
    for (size_t i = 0; i < oversamplers.size(); ++i)
    {
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>> (
            (size_t) numCh,
            i + 1,                                                   // factor = log2(ratio)
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
            true,   // max quality
            true);  // integer latency

        oversamplers[i]->initProcessing ((size_t) samplesPerBlock);
        oversamplers[i]->reset();
    }

    // The nonlinear stages are sized for the worst case (8x) so switching
    // oversampling at runtime never allocates on the audio thread.
    juce::dsp::ProcessSpec osSpec { sampleRate * 8.0,
                                    (juce::uint32) (samplesPerBlock * 8),
                                    numCh };
    push.prepare (osSpec);
    saturator.prepare (osSpec);

    currentOsChoice = -1;
    reportedLatency = -1;

    updateParameters();
    updateLatency();
}

void ZinoxVocalsProcessor::releaseResources()
{
    denoiser.reset();
    toneStack.reset();
    deEsser.reset();
    control.reset();
    push.reset();
    saturator.reset();
    limiter.reset();

    for (auto& os : oversamplers)
        if (os != nullptr)
            os->reset();
}

bool ZinoxVocalsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return in == out;
}

// ---------------------------------------------------------------------------

void ZinoxVocalsProcessor::updateParameters()
{
    // --- denoiser -------------------------------------------------------------
    denoiser.setAmount (pDenoise->load());

    // --- tone ---------------------------------------------------------------
    toneStack.setParams (pLowCut->load(), pHighCut->load(),
                         pLowGain->load(), pLowFreq->load(),
                         (dsp::ToneStack::LowShape) (int) pLowShape->load(),
                         pMidGain->load(), pMidTone->load(),
                         pHighGain->load(),
                         (dsp::ToneStack::HighMode) (int) pHighMode->load());

    // --- de-esser -----------------------------------------------------------
    deEsser.setParams (pDeEss->load(), pDeEssFreq->load());

    // --- control compressor -------------------------------------------------
    {
        const auto amount = pControl->load();
        const auto rangeDb = rangeIndexToDb ((int) pControlRange->load());
        const auto mode = (int) pControlMode->load();

        dsp::Compressor::Settings s;

        switch (mode)
        {
            case 0:  // AGGRO — fast and obvious, grabs every syllable
                s.attackMs = 1.5f;  s.releaseMs = 60.0f;  s.ratio = 8.0f;  s.kneeDb = 3.0f;
                s.rmsDetect = false;
                break;
            case 1:  // SMOOTH — opto-style, slow and forgiving
                s.attackMs = 25.0f; s.releaseMs = 320.0f; s.ratio = 3.0f;  s.kneeDb = 12.0f;
                s.rmsDetect = true;
                break;
            default: // VOCAL — the middle ground, tuned for lead vocals
                s.attackMs = 8.0f;  s.releaseMs = 140.0f; s.ratio = 4.5f;  s.kneeDb = 8.0f;
                s.rmsDetect = true;
                break;
        }

        s.thresholdDb = juce::jmap (amount, 0.0f, 1.0f, -4.0f, -36.0f);
        s.maxGrDb     = rangeDb;
        // Auto make-up: give back most of what the range is taking away.
        s.makeupDb    = amount * rangeDb * 0.8f;

        control.setSettings (s);
    }

    // --- push ---------------------------------------------------------------
    push.setParams (pPush->load(),
                    (dsp::Push::Mode) (int) pPushMode->load(),
                    rangeIndexToDb ((int) pPushRange->load()));

    // --- saturation ---------------------------------------------------------
    saturator.setParams (pSaturate->load(), (dsp::Saturator::Mode) (int) pSatMode->load());

    // --- gains / limiter ----------------------------------------------------
    inputGain.setGainDecibels (pInput->load());
    outputGain.setGainDecibels (pOutput->load());

    limiter.setCeiling (pLimit->load());
    limiter.setEnabled (pLimitOn->load() > 0.5f);
}

void ZinoxVocalsProcessor::updateLatency()
{
    int latency = limiter.getLatencySamples() + denoiser.getLatencySamples();

    const auto osChoice = (int) pOversample->load();
    if (osChoice > 0 && oversamplers[(size_t) (osChoice - 1)] != nullptr)
        latency += (int) oversamplers[(size_t) (osChoice - 1)]->getLatencyInSamples();

    if (latency != reportedLatency)
    {
        reportedLatency = latency;
        setLatencySamples (latency);
    }
}

void ZinoxVocalsProcessor::applyOversampledStage (juce::dsp::AudioBlock<float>& block,
                                                  double stageSampleRate)
{
    push.setSampleRate (stageSampleRate);
    saturator.setSampleRate (stageSampleRate);

    push.process (block);
    saturator.process (block);
}

// ---------------------------------------------------------------------------

void ZinoxVocalsProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    const auto totalIn  = getTotalNumInputChannels();
    const auto totalOut = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int ch = totalIn; ch < totalOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numSamples == 0)
        return;

    updateParameters();

    // Cheap getters guarded by a change check — setLatencySamples() only ever
    // reaches the host when the reported value actually moves.
    updateLatency();

    // The free trial is a real, full-quality 7 days - no watermarking, no
    // nagging. Once it's over and no license has been entered, the plugin
    // goes silent rather than quietly degrading, so there's never any doubt
    // about why.
    if (trialBlocked.load (std::memory_order_relaxed))
    {
        buffer.clear();
        meters.outLeft.store (-100.0f);
        meters.outRight.store (-100.0f);
        meters.limitGr.store (0.0f);
        meters.deEssGr.store (0.0f);
        meters.controlGr.store (0.0f);
        meters.pushGr.store (0.0f);
        return;
    }

    if (pBypass->load() > 0.5f)
    {
        meters.outLeft.store (-100.0f);
        meters.outRight.store (-100.0f);
        meters.limitGr.store (0.0f);
        meters.deEssGr.store (0.0f);
        meters.controlGr.store (0.0f);
        meters.pushGr.store (0.0f);
        return;
    }

    // --- keep a dry copy for the mix control --------------------------------
    const auto mix = pMix->load();
    const bool needsDry = mix < 0.999f;

    if (needsDry)
    {
        dryBuffer.setSize (totalOut, numSamples, false, false, true);
        for (int ch = 0; ch < totalOut; ++ch)
            dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    // 1. input trim ----------------------------------------------------------
    inputGain.process (ctx);

    // 2. adaptive denoiser -----------------------------------------------------
    denoiser.process (buffer);

    // 3. filters + tone ------------------------------------------------------
    toneStack.process (buffer);

    // 4. de-esser ------------------------------------------------------------
    deEsser.process (buffer);
    meters.deEssGr.store (deEsser.getGainReductionDb());

    // 5. control compressor --------------------------------------------------
    control.process (block);
    meters.controlGr.store (control.getGainReductionDb());

    // 6. push + saturate, optionally oversampled -----------------------------
    {
        const auto osChoice = juce::jlimit (0, 3, (int) pOversample->load());
        currentOsChoice = osChoice;

        if (osChoice == 0)
        {
            applyOversampledStage (block, hostSampleRate);
        }
        else
        {
            auto& os = *oversamplers[(size_t) (osChoice - 1)];
            auto upBlock = os.processSamplesUp (block);
            const auto ratio = std::pow (2.0, (double) osChoice);
            applyOversampledStage (upBlock, hostSampleRate * ratio);
            os.processSamplesDown (block);
        }

        meters.pushGr.store (push.getGainReductionDb());
    }

    // 7. output trim ---------------------------------------------------------
    outputGain.process (ctx);

    // 8. brickwall limiter ---------------------------------------------------
    limiter.process (buffer);
    meters.limitGr.store (limiter.getGainReductionDb());

    // 9. dry/wet -------------------------------------------------------------
    if (needsDry)
    {
        for (int ch = 0; ch < totalOut; ++ch)
        {
            auto* wet = buffer.getWritePointer (ch);
            auto* dry = dryBuffer.getReadPointer (ch);

            for (int i = 0; i < numSamples; ++i)
                wet[i] = wet[i] * mix + dry[i] * (1.0f - mix);
        }
    }

    // --- metering -----------------------------------------------------------
    const auto peakL = buffer.getMagnitude (0, 0, numSamples);
    const auto peakR = totalOut > 1 ? buffer.getMagnitude (1, 0, numSamples) : peakL;

    meters.outLeft.store (juce::Decibels::gainToDecibels (peakL, -100.0f));
    meters.outRight.store (juce::Decibels::gainToDecibels (peakR, -100.0f));
}

// ---------------------------------------------------------------------------

juce::AudioProcessorEditor* ZinoxVocalsProcessor::createEditor()
{
    return new ZinoxVocalsEditor (*this);
}

void ZinoxVocalsProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presetManager.getCurrentPresetName(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ZinoxVocalsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            apvts.replaceState (tree);
            presetManager.setCurrentPresetName (tree.getProperty ("presetName", "Default").toString());
        }
    }
}

// ---------------------------------------------------------------------------

zx::License::Info ZinoxVocalsProcessor::activateLicense (const juce::String& licenseBlob)
{
    licenseInfo = zx::License::activate (licenseBlob);
    updateTrialBlockedFlag();
    return licenseInfo;
}

void ZinoxVocalsProcessor::refreshTrialStatus()
{
    trialStatus = zx::Trial::checkAndUpdate();
    updateTrialBlockedFlag();
}

void ZinoxVocalsProcessor::updateTrialBlockedFlag() noexcept
{
    trialBlocked.store (! licenseInfo.valid && trialStatus.expired, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ZinoxVocalsProcessor();
}
