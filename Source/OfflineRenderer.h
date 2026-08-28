#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace zx
{

/**
    Bounces an audio file through a throwaway instance of the plugin,
    configured with a snapshot of the live instance's current parameter
    state, so the exported file matches exactly what you'd hear right now.

    Compensates for the plugin's own reported latency (limiter look-ahead,
    the denoiser's STFT window, any oversampling) by feeding it a little
    extra silence at the end and discarding the corresponding delay at the
    start, so the output file lines up sample-for-sample with the input and
    is exactly the same length.

    Runs on a background thread behind a modal progress window - most vocal
    takes render in well under a second, but a long file won't freeze the UI.

    Plugin builds don't permit the blocking, synchronous runThread() (DAWs
    can't have a plugin block their message thread in a modal loop), so this
    uses launchThread() instead: construct it with `new` and call
    launchThread(); threadComplete() reports the result and deletes the
    object itself once rendering finishes - nothing else to manage.
*/
class OfflineRenderer : public juce::ThreadWithProgressWindow
{
public:
    OfflineRenderer (const juce::File& sourceFile, const juce::File& destFile,
                     juce::MemoryBlock stateToApply);

    void run() override;
    void threadComplete (bool userPressedCancel) override;

    bool succeeded() const noexcept { return success; }
    const juce::String& getErrorMessage() const noexcept { return errorMessage; }

private:
    juce::File source, dest;
    juce::MemoryBlock state;
    bool success = false;
    juce::String errorMessage;
};

} // namespace zx
