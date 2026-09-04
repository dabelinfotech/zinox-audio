#include "Licensing.h"
#include <juce_cryptography/juce_cryptography.h>

namespace zx
{

// The public half of the RSA key pair created by
// `ZinoxLicenseGen genkeys` (see Source/Tools/LicenseKeyGen.cpp). Only the
// public key lives in source control - it can verify a signature but never
// create one, so shipping it in the plugin binary is safe.
//
// This placeholder will reject every key, which is the correct fail-closed
// state until you actually generate a key pair. Run the generator once,
// then paste its "PUBLIC KEY" output here and rebuild.
static constexpr const char* kPublicKey = "";

namespace
{
    juce::String canonicalFields (const juce::String& name, const juce::String& email,
                                  const juce::String& machine, const juce::String& expires)
    {
        return name + "\n" + email + "\n" + machine + "\n" + expires + "\n";
    }

    // Mirrors Trial's anti-rollback high-water mark (see Trial.cpp): winding
    // the system clock backwards can't resurrect an expired subscription key,
    // because the expiry check always uses the latest clock value this
    // installation has ever observed.
    juce::Time protectedNow()
    {
        auto file = License::getLicenseFile().getSiblingFile ("license-clock.dat");
        const auto now = juce::Time::currentTimeMillis();

        juce::int64 highWaterMark = now;
        if (file.existsAsFile())
            highWaterMark = file.loadFileAsString().getLargeIntValue();

        const auto effectiveNow = juce::jmax (now, highWaterMark);

        file.getParentDirectory().createDirectory();
        file.replaceWithText (juce::String (effectiveNow));

        return juce::Time (effectiveNow);
    }
}

juce::String License::getMachineId()
{
    return juce::SystemStats::getUniqueDeviceID();
}

juce::File License::getLicenseFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Zinox Audio")
               .getChildFile ("Zinox Vocals")
               .getChildFile ("license.key");
}

License::Info License::verify (const juce::String& licenseBlob, juce::Time now)
{
    Info info;

    const auto trimmed = licenseBlob.trim();
    if (trimmed.isEmpty())
        return info;

    juce::RSAKey publicKey (kPublicKey);
    if (! publicKey.isValid())
        return info; // no key pair has been generated for this build yet

    juce::MemoryOutputStream decoded;
    if (! juce::Base64::convertFromBase64 (decoded, trimmed))
        return info;

    auto xml = juce::XmlDocument::parse (decoded.toString());
    if (xml == nullptr || ! xml->hasTagName ("ZINOXLICENSE"))
        return info;

    const auto name    = xml->getStringAttribute ("name");
    const auto email   = xml->getStringAttribute ("email");
    const auto machine = xml->getStringAttribute ("machine");
    const auto expires = xml->getStringAttribute ("expires");
    const auto sigHex   = xml->getStringAttribute ("sig");

    if (name.isEmpty() || email.isEmpty() || sigHex.isEmpty())
        return info;

    // A key issued with a machine ID only unlocks that one machine.
    if (machine.isNotEmpty() && machine != getMachineId())
        return info;

    juce::SHA256 hash (canonicalFields (name, email, machine, expires).toUTF8());

    juce::BigInteger hashValue, signature;
    hashValue.parseString (hash.toHexString(), 16);
    signature.parseString (sigHex, 16);

    if (! publicKey.applyToValue (signature) || signature != hashValue)
        return info;

    // Signature confirmed authentic below this point, so it's safe to
    // surface name/email even if the key turns out to be expired.
    info.name = name;
    info.email = email;

    if (expires.isNotEmpty())
    {
        const auto expiresAt = juce::Time::fromISO8601 (expires);

        // A blank/unparseable date on an otherwise-authentic signature means
        // the field was mangled somehow - fail closed rather than guess.
        if (expiresAt == juce::Time())
            return Info();

        info.hasExpiry = true;
        info.expiresAt = expiresAt;

        if (now >= expiresAt)
        {
            info.expired = true;
            return info; // valid stays false: expired keys don't authorize use
        }
    }

    info.valid = true;
    return info;
}

License::Info License::loadSaved()
{
    auto file = getLicenseFile();

    if (! file.existsAsFile())
        return {};

    return verify (file.loadFileAsString(), protectedNow());
}

License::Info License::activate (const juce::String& licenseBlob)
{
    auto info = verify (licenseBlob, protectedNow());

    if (info.valid)
    {
        auto file = getLicenseFile();
        file.getParentDirectory().createDirectory();
        file.replaceWithText (licenseBlob.trim());
    }

    return info;
}

} // namespace zx
