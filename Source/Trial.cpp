#include "Trial.h"

namespace zx
{

juce::File Trial::getTrialFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Zinox Audio")
               .getChildFile ("Zinox Vocals")
               .getChildFile ("trial.dat");
}

Trial::Status Trial::checkAndUpdate()
{
    auto file = getTrialFile();
    const auto now = juce::Time::currentTimeMillis();

    juce::int64 firstRun = now;
    juce::int64 highWaterMark = now;

    if (file.existsAsFile())
    {
        const auto lines = juce::StringArray::fromLines (file.loadFileAsString());

        if (lines.size() >= 2)
        {
            firstRun      = lines[0].getLargeIntValue();
            highWaterMark = lines[1].getLargeIntValue();
        }
    }
    else
    {
        file.getParentDirectory().createDirectory();
    }

    // Never let the effective clock move backwards, so rolling the system
    // date back can't buy the trial more time.
    const auto effectiveNow = juce::jmax (now, highWaterMark);

    file.replaceWithText (juce::String (firstRun) + "\n" + juce::String (effectiveNow));

    const auto elapsedMs = juce::jmax ((juce::int64) 0, effectiveNow - firstRun);
    const auto msPerDay  = (juce::int64) 1000 * 60 * 60 * 24;

    Status status;
    status.daysUsed      = (int) (elapsedMs / msPerDay);
    status.daysRemaining = juce::jmax (0, kTrialDays - status.daysUsed);
    status.expired       = status.daysUsed >= kTrialDays;
    return status;
}

} // namespace zx
