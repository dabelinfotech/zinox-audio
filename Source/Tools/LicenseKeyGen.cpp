/*
    Zinox License Key Generator
    ---------------------------
    A standalone command-line tool - NOT part of the plugin, and NOT shipped
    to customers - used to create the RSA key pair and to issue signed
    license keys.

    The private key it creates must never be committed to source control or
    shared with anyone: whoever holds it can mint valid keys for Zinox
    Vocals. Keep it somewhere safe (a password manager, an encrypted drive).

    Usage:
      ZinoxLicenseGen genkeys
          Creates a new RSA key pair under Licensing/keys/ (gitignored).
          Run this exactly ONCE per product. Re-running invalidates every
          key already issued, since old keys won't verify against a new
          public key.

      ZinoxLicenseGen issue "Customer Name" "customer@email.com" <duration> [--machine <id>]
          Prints a signed license key for that customer. <duration> is
          exactly one of:
              --monthly       expires 30 days from now
              --yearly        expires 365 days from now
              --days N        expires N days from now (e.g. a renewal)
              --perpetual     never expires (one-time purchase, not a sub)
          Pass --machine with the ID shown in the plugin's license dialog to
          lock the key to one machine, or omit it to issue a key that works
          anywhere.

      ZinoxLicenseGen verify "<key blob>"
          Checks a key against the public key currently compiled into
          Source/Licensing.cpp - handy for confirming a key works before
          you send it to a customer.
*/

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include "../Licensing.h"
#include <iostream>

using namespace juce;

namespace
{
    File keysDir()
    {
        return File::getCurrentWorkingDirectory().getChildFile ("Licensing").getChildFile ("keys");
    }

    [[noreturn]] void printUsageAndExit()
    {
        std::cout << "Usage:\n"
                     "  ZinoxLicenseGen genkeys\n"
                     "  ZinoxLicenseGen issue \"Name\" \"email@example.com\" <duration> [--machine <id>]\n"
                     "      <duration> is exactly one of: --monthly | --yearly | --days N | --perpetual\n"
                     "  ZinoxLicenseGen verify \"<key blob>\"\n";
        std::exit (1);
    }

    void cmdGenKeys()
    {
        auto dir = keysDir();
        dir.createDirectory();

        auto pubFile  = dir.getChildFile ("public.key");
        auto privFile = dir.getChildFile ("private.key");

        if (pubFile.existsAsFile() || privFile.existsAsFile())
        {
            std::cout << "A key pair already exists in " << dir.getFullPathName() << "\n"
                         "Delete both files first if you really want to replace them - doing so\n"
                         "invalidates every key you have already issued to customers.\n";
            std::exit (1);
        }

        std::cout << "Generating a 512-bit RSA key pair (may take a few seconds)...\n";

        RSAKey publicKey, privateKey;
        RSAKey::createKeyPair (publicKey, privateKey, 512);

        pubFile.replaceWithText (publicKey.toString());
        privFile.replaceWithText (privateKey.toString());

        std::cout << "\nDone. Keys written to " << dir.getFullPathName() << "\n\n"
                     "Next step: copy the PUBLIC key below into kPublicKey in\n"
                     "Source/Licensing.cpp, then rebuild the plugin.\n\n"
                     "PUBLIC KEY:\n" << publicKey.toString() << "\n\n"
                     "private.key is your secret. Never commit it, never share it.\n";
    }

    void cmdIssue (const String& name, const String& email, const String& machineId,
                   bool hasExpiry, Time expiresAt)
    {
        auto privFile = keysDir().getChildFile ("private.key");

        if (! privFile.existsAsFile())
        {
            std::cout << "No private key found. Run 'ZinoxLicenseGen genkeys' first.\n";
            std::exit (1);
        }

        RSAKey privateKey (privFile.loadFileAsString());

        if (! privateKey.isValid())
        {
            std::cout << "Licensing/keys/private.key is not a valid key.\n";
            std::exit (1);
        }

        const auto expires = hasExpiry ? expiresAt.toISO8601 (true) : String();

        const auto canonical = name + "\n" + email + "\n" + machineId + "\n" + expires + "\n";
        SHA256 hash (canonical.toUTF8());

        BigInteger hashValue;
        hashValue.parseString (hash.toHexString(), 16);

        auto signature = hashValue;
        privateKey.applyToValue (signature);

        XmlElement xml ("ZINOXLICENSE");
        xml.setAttribute ("name", name);
        xml.setAttribute ("email", email);
        xml.setAttribute ("machine", machineId);
        xml.setAttribute ("expires", expires);
        xml.setAttribute ("sig", signature.toString (16));

        const auto key = Base64::toBase64 (xml.toString());

        std::cout << "\nLicense key for " << name << " <" << email << ">"
                  << (machineId.isNotEmpty() ? " (locked to one machine, " : " (works on any machine, ")
                  << (hasExpiry ? "expires " + expiresAt.formatted ("%d %b %Y") : String ("never expires"))
                  << "):\n\n" << key << "\n\n";
    }

    void cmdVerify (const String& blob)
    {
        const auto info = zx::License::verify (blob);

        if (info.valid)
        {
            std::cout << "VALID - " << info.name << " <" << info.email << ">"
                       << (info.hasExpiry ? " - expires " + info.expiresAt.formatted ("%d %b %Y") : String (" - never expires"))
                       << "\n";
        }
        else if (info.expired)
        {
            std::cout << "EXPIRED - " << info.name << " <" << info.email << "> - expired "
                       << info.expiresAt.formatted ("%d %b %Y") << "\n";
        }
        else
        {
            std::cout << "INVALID - this key does not verify against the public key compiled "
                         "into Source/Licensing.cpp (or it's locked to a different machine).\n";
        }
    }
}

int main (int argc, char* argv[])
{
    if (argc < 2)
        printUsageAndExit();

    const String command (argv[1]);

    if (command == "genkeys")
    {
        cmdGenKeys();
    }
    else if (command == "issue")
    {
        if (argc < 5)
            printUsageAndExit();

        const String name (argv[2]);
        const String email (argv[3]);

        String machineId;
        bool hasExpiry = false, sawDurationFlag = false;
        Time expiresAt;

        for (int i = 4; i < argc; ++i)
        {
            const String arg (argv[i]);

            if (arg == "--machine")
            {
                if (++i >= argc)
                    printUsageAndExit();
                machineId = argv[i];
            }
            else if (arg == "--perpetual")
            {
                if (sawDurationFlag)
                    printUsageAndExit();
                sawDurationFlag = true;
                hasExpiry = false;
            }
            else if (arg == "--monthly" || arg == "--yearly")
            {
                if (sawDurationFlag)
                    printUsageAndExit();
                sawDurationFlag = true;
                hasExpiry = true;
                expiresAt = Time::getCurrentTime() + RelativeTime::days (arg == "--monthly" ? 30 : 365);
            }
            else if (arg == "--days")
            {
                if (sawDurationFlag || ++i >= argc)
                    printUsageAndExit();
                sawDurationFlag = true;
                hasExpiry = true;
                expiresAt = Time::getCurrentTime() + RelativeTime::days (String (argv[i]).getIntValue());
            }
            else
            {
                printUsageAndExit();
            }
        }

        if (! sawDurationFlag)
            printUsageAndExit();

        cmdIssue (name, email, machineId, hasExpiry, expiresAt);
    }
    else if (command == "verify")
    {
        if (argc < 3)
            printUsageAndExit();

        cmdVerify (argv[2]);
    }
    else
    {
        printUsageAndExit();
    }

    return 0;
}
