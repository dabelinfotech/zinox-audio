#pragma once

#include <juce_core/juce_core.h>

namespace zx
{

/**
    A 7-day, fully-offline free trial.

    First launch records a start time to disk. Every launch after that
    compares against it. There is no server and no license check involved
    here at all - this purely answers "has it been more than 7 days since
    this copy was first run", for use alongside (not instead of) License.

    A high-water mark on the recorded time means winding the system clock
    backwards can't extend the trial - the elapsed-time calculation always
    uses the latest clock value this installation has ever observed.
*/
class Trial
{
public:
    static constexpr int kTrialDays = 7;

    struct Status
    {
        int  daysUsed = 0;
        int  daysRemaining = kTrialDays;
        bool expired = false;
    };

    static juce::File getTrialFile();

    /** Call once per run. Creates the trial record on first launch, updates
        the anti-rollback high-water mark, and returns where things stand. */
    static Status checkAndUpdate();
};

} // namespace zx
