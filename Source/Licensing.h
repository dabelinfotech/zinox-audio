#pragma once

#include <juce_core/juce_core.h>

namespace zx
{

/**
    Offline, no-server license verification.

    A license key is a base64 blob signed with an RSA private key that never
    ships with the plugin - only the matching public key (embedded in
    Licensing.cpp) does, so the plugin can verify a key but never mint one.
    Keys are produced by the separate LicenseKeyGen command-line tool
    (Source/Tools/LicenseKeyGen.cpp), run by hand whenever a sale comes in.

    There is no server and no phone-home: verification is a pure local
    signature check, so the plugin works fully offline once a key has been
    entered.

    A key can optionally carry a signed expiry date, for subscription sales -
    an expired key is cryptographically distinguishable from an invalid one
    (Info::expired vs. a plain invalid Info), so the UI can prompt for
    renewal rather than just saying the key is bad. Rolling the system clock
    back can't extend an expired key past its date; see the high-water-mark
    handling in loadSaved()/activate().
*/
class License
{
public:
    struct Info
    {
        bool valid = false;

        /** True when the signature checks out but the key's expiry date has
            passed - a distinct state from an invalid/tampered key, so the UI
            can tell a lapsed subscription apart from a bad key. */
        bool expired = false;

        bool hasExpiry = false;
        juce::Time expiresAt; // only meaningful when hasExpiry is true

        juce::String name, email;
    };

    /** Checks a key's signature, its machine lock (if it has one), and its
        expiry date (if it has one) against `now`, without touching disk.
        `now` defaults to the live system clock; callers that need
        clock-rollback protection (see loadSaved()/activate()) pass in a
        high-water-marked time instead. */
    static Info verify (const juce::String& licenseBlob,
                         juce::Time now = juce::Time::getCurrentTime());

    /** A stable per-machine identifier, shown in the UI so a customer can
        send it to you for a machine-locked key. */
    static juce::String getMachineId();

    static juce::File getLicenseFile();

    /** Re-verifies whatever key was saved from a previous activation. */
    static Info loadSaved();

    /** Verifies a key and, if valid, persists it so loadSaved() finds it
        next time. Returns the parsed info either way. */
    static Info activate (const juce::String& licenseBlob);
};

} // namespace zx
