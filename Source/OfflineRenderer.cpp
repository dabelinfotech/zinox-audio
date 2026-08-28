#include "OfflineRenderer.h"
#include "PluginProcessor.h"

namespace zx
{

OfflineRenderer::OfflineRenderer (const juce::File& sourceFile, const juce::File& destFile,
                                  juce::MemoryBlock stateToApply)
    : ThreadWithProgressWindow ("Rendering...", true, true),
      source (sourceFile), dest (destFile), state (std::move (stateToApply))
{
}

void OfflineRenderer::run()
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (source));

    if (reader == nullptr)
    {
        errorMessage = "Couldn't open \"" + source.getFileName() + "\" - unrecognised or corrupt audio file.";
        return;
    }

    // A fresh, throwaway processor instance loaded with a snapshot of the
    // live one's parameters. Rendering here can never disturb whatever is
    // playing live through the standalone app or a host.
    ZinoxVocalsProcessor offlineProcessor;
    offlineProcessor.setStateInformation (state.getData(), (int) state.getSize());
    offlineProcessor.setNonRealtime (true);

    constexpr int blockSize = 4096;
    offlineProcessor.prepareToPlay (reader->sampleRate, blockSize);

    auto outStream = dest.createOutputStream();

    if (outStream == nullptr)
    {
        errorMessage = "Couldn't create \"" + dest.getFileName() + "\".";
        offlineProcessor.releaseResources();
        return;
    }

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer (
        wavFormat.createWriterFor (outStream.get(), reader->sampleRate,
                                   reader->numChannels, 24, {}, 0));

    if (writer == nullptr)
    {
        errorMessage = "Couldn't create a WAV writer for \"" + dest.getFileName() + "\".";
        offlineProcessor.releaseResources();
        return;
    }

    outStream.release(); // the writer now owns the stream

    // Feed the plugin's own reported latency worth of extra silence at the
    // end so its internal delay lines flush, then discard that same amount
    // of leading silence from the output - the result lines up with the
    // input and is exactly the same length.
    const juce::int64 latency        = offlineProcessor.getLatencySamples();
    const juce::int64 originalLength = reader->lengthInSamples;
    const juce::int64 totalToFeed    = originalLength + latency;

    juce::AudioBuffer<float> buffer ((int) reader->numChannels, blockSize);
    juce::MidiBuffer midi;

    juce::int64 readPos = 0;
    juce::int64 produced = 0;

    while (produced < totalToFeed)
    {
        if (threadShouldExit())
        {
            errorMessage = "Cancelled.";
            offlineProcessor.releaseResources();
            return;
        }

        const auto samplesThisBlock = (int) juce::jmin ((juce::int64) blockSize, totalToFeed - produced);

        buffer.setSize (buffer.getNumChannels(), samplesThisBlock, false, false, true);
        buffer.clear();

        if (readPos < originalLength)
        {
            const auto numToRead = (int) juce::jmin ((juce::int64) samplesThisBlock, originalLength - readPos);
            reader->read (&buffer, 0, numToRead, readPos, true, true);
            readPos += numToRead;
        }

        midi.clear();
        offlineProcessor.processBlock (buffer, midi);

        const auto blockStart = produced;
        const auto blockEnd   = produced + samplesThisBlock;
        const auto outStart   = juce::jmax (blockStart, latency);
        const auto outEnd     = juce::jmin (blockEnd, latency + originalLength);

        if (outEnd > outStart)
        {
            writer->writeFromAudioSampleBuffer (buffer, (int) (outStart - blockStart),
                                                (int) (outEnd - outStart));
        }

        produced += samplesThisBlock;
        setProgress ((double) produced / (double) juce::jmax ((juce::int64) 1, totalToFeed));
    }

    offlineProcessor.releaseResources();
    success = true;
}

void OfflineRenderer::threadComplete (bool userPressedCancel)
{
    if (! userPressedCancel)
    {
        if (success)
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::InfoIcon, "Export Complete",
                "Saved to:\n" + dest.getFullPathName());
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::WarningIcon, "Export Failed", errorMessage);
        }
    }

    delete this;
}

} // namespace zx
