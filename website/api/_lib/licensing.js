// Server-side counterpart to Source/Licensing.cpp + Source/Tools/LicenseKeyGen.cpp -
// signs a license key the same way the offline CLI tool does, byte-for-byte
// compatible, so a plugin verifies a server-issued key exactly like a
// locally-issued one. This exists so the admin panel can issue a key from
// any browser without the CLI tool or the private key file being present on
// that machine.
//
// This is a real security trade-off, not a detail: unlike the CLI path,
// running here means the private key lives in a Vercel environment
// variable. See website/api/README.md for which env var each plugin uses
// and how to set one up. The CLI/local path (Source/Tools/LicenseKeyGen.cpp)
// still exists unchanged and doesn't require any of this.
const crypto = require('crypto');

function hexToBigInt(hex) {
  return BigInt('0x' + hex);
}

// Plain modular exponentiation - deliberately not using Node's crypto.sign(),
// because JUCE's RSAKey::applyToValue does raw/textbook RSA (signature =
// hash^d mod n, no PKCS#1 padding), which crypto.sign() doesn't produce.
function modPow(base, exponent, modulus) {
  let result = 1n;
  base %= modulus;
  while (exponent > 0n) {
    if (exponent & 1n) result = (result * base) % modulus;
    exponent >>= 1n;
    base = (base * base) % modulus;
  }
  return result;
}

// Parses the "hexPart1,hexPart2" format produced by JUCE's RSAKey::toString().
// For a private key, part1 is the exponent (d) and part2 is the modulus (n).
function parseKey(keyString) {
  const trimmed = (keyString || '').trim();
  const commaIndex = trimmed.indexOf(',');
  if (commaIndex === -1) throw new Error('Malformed RSA key - expected "hex,hex"');
  return {
    exponent: hexToBigInt(trimmed.slice(0, commaIndex)),
    modulus: hexToBigInt(trimmed.slice(commaIndex + 1)),
  };
}

function escapeXmlAttr(value) {
  return String(value)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

/** Maps a plugin slug to the env var holding its RSA private key, e.g.
 *  "zinox-vocals" -> "LICENSE_PRIVATE_KEY_ZINOX_VOCALS". Each plugin has its
 *  own key pair (see that plugin's own LicenseKeyGen tool), so each needs its
 *  own env var - there is no single shared signing key. */
function envVarNameForSlug(slug) {
  return 'LICENSE_PRIVATE_KEY_' + slug.toUpperCase().replace(/[^A-Z0-9]+/g, '_');
}

/** Signs a license the same way `<Tool> issue` does. `expiresAt`, if given,
 *  must be a Date - omit it (or pass null) for a perpetual key. Throws if
 *  `privateKeyString` isn't set or isn't parseable. */
function signLicense({ name, email, machineId = '', expiresAt = null }, privateKeyString) {
  const { exponent, modulus } = parseKey(privateKeyString);

  const expires = expiresAt ? expiresAt.toISOString() : '';
  const canonical = `${name}\n${email}\n${machineId}\n${expires}\n`;

  const hashHex = crypto.createHash('sha256').update(canonical, 'utf8').digest('hex');
  const signature = modPow(hexToBigInt(hashHex), exponent, modulus);

  const xml = '<?xml version="1.0" encoding="UTF-8"?>\n'
    + `<ZINOXLICENSE name="${escapeXmlAttr(name)}" email="${escapeXmlAttr(email)}" `
    + `machine="${escapeXmlAttr(machineId)}" expires="${escapeXmlAttr(expires)}" `
    + `sig="${signature.toString(16)}"/>`;

  return Buffer.from(xml, 'utf8').toString('base64');
}

/** Turns a CLI-style duration selector into an expiry Date (or null for
 *  perpetual), matching --monthly/--yearly/--days/--perpetual. */
function resolveExpiry(duration, days) {
  const now = Date.now();
  const DAY_MS = 24 * 60 * 60 * 1000;
  if (duration === 'monthly') return new Date(now + 30 * DAY_MS);
  if (duration === 'yearly') return new Date(now + 365 * DAY_MS);
  if (duration === 'perpetual') return null;
  if (duration === 'days') {
    const n = Number(days);
    if (!Number.isFinite(n) || n <= 0) throw new Error('days must be a positive number');
    return new Date(now + n * DAY_MS);
  }
  throw new Error('duration must be one of: monthly, yearly, days, perpetual');
}

module.exports = { signLicense, resolveExpiry, envVarNameForSlug };
