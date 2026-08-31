-- ZinoxAudio admin backend schema.
-- Run this once against your Vercel Postgres database (Storage tab -> your
-- database -> Query) before using the admin panel. Safe to re-run - every
-- statement is idempotent.

CREATE TABLE IF NOT EXISTS plugins (
  id SERIAL PRIMARY KEY,
  slug TEXT UNIQUE NOT NULL,
  name TEXT NOT NULL,
  description TEXT,
  price_monthly_cents INTEGER,
  price_yearly_cents INTEGER,
  screenshot_url TEXT,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- One row per uploaded installer. is_current marks the file /api/download
-- serves for that plugin+platform; uploading a new one flips the previous
-- current row to false rather than deleting it, so history isn't lost.
CREATE TABLE IF NOT EXISTS plugin_files (
  id SERIAL PRIMARY KEY,
  plugin_id INTEGER NOT NULL REFERENCES plugins(id) ON DELETE CASCADE,
  platform TEXT NOT NULL CHECK (platform IN ('windows', 'macos')),
  version TEXT NOT NULL,
  blob_url TEXT NOT NULL,
  size_bytes BIGINT,
  is_current BOOLEAN NOT NULL DEFAULT true,
  uploaded_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_plugin_files_current
  ON plugin_files (plugin_id, platform)
  WHERE is_current;

CREATE TABLE IF NOT EXISTS subscribers (
  id SERIAL PRIMARY KEY,
  name TEXT,
  email TEXT UNIQUE NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- Tracks license keys issued via the offline HeliumLicenseGen / ZinoxLicenseGen
-- CLI tools (see Source/Tools/LicenseKeyGen.cpp). The signing private key
-- never touches this server - an admin runs the CLI locally, then pastes the
-- resulting key blob in here purely as a record, so it shows up in the
-- admin panel and can be looked up or marked revoked later.
CREATE TABLE IF NOT EXISTS license_tokens (
  id SERIAL PRIMARY KEY,
  subscriber_id INTEGER REFERENCES subscribers(id) ON DELETE SET NULL,
  plugin_id INTEGER NOT NULL REFERENCES plugins(id) ON DELETE CASCADE,
  key_blob TEXT NOT NULL,
  machine_id TEXT,
  revoked BOOLEAN NOT NULL DEFAULT false,
  issued_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- One row per /api/download hit. ip_hash is a salted hash, not a raw IP -
-- enough to de-duplicate/estimate reach without storing identifying data
-- (keeps this consistent with privacy.html's "no telemetry" promise, which
-- is about the plugin binary itself, not this website's own download log).
CREATE TABLE IF NOT EXISTS download_events (
  id SERIAL PRIMARY KEY,
  plugin_id INTEGER NOT NULL REFERENCES plugins(id) ON DELETE CASCADE,
  platform TEXT NOT NULL,
  subscriber_id INTEGER REFERENCES subscribers(id) ON DELETE SET NULL,
  ip_hash TEXT,
  downloaded_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- Seed rows for the products this site already lists. Adjust pricing/
-- description from the admin panel at any time - this just gets you started.
INSERT INTO plugins (slug, name, description, price_monthly_cents, price_yearly_cents)
VALUES
  ('zinox-vocals', 'Zinox Vocals', 'Vocal channel strip - VST3 / AU / Standalone', 500, 5000),
  ('helium', 'Helium', 'High-frequency enhancer - VST3 / AU / Standalone', 800, 6000),
  ('zinox-lacquer', 'Zinox Lacquer', 'Modular drag-and-drop vocal rack - VST3 / AU / Standalone', 800, 6000)
ON CONFLICT (slug) DO NOTHING;
