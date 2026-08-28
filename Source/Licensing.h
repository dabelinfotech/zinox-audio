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
*/
class License
{
public:
    struct Info
    {
        bool valid = false;
        juce::String name, email;
    };

    /** Checks a key's signature (and its machine lock, if it has one)
        without touching disk. */
    static Info verify (const juce::String& licenseBlob);

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
