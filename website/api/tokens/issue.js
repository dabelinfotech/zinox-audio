// Signs AND logs a license key in one step, as an alternative to the manual
// "run the CLI locally, paste the result into /api/tokens" flow in
// tokens/[[...id]].js - that path is untouched and still works exactly as
// before. This one lets the admin panel issue a key from any browser, at
// the cost of the plugin's private key living in this function's
// environment instead of only on whoever's machine runs the CLI. See
// website/api/README.md for which env var each plugin's key lives in.
const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');
const { signLicense, resolveExpiry, envVarNameForSlug } = require('../_lib/licensing');

module.exports = async (req, res) => {
  if (!requireAdmin(req, res)) return;

  if (req.method !== 'POST') {
    res.status(405).json({ error: 'Method not allowed' });
    return;
  }

  const { pluginSlug, name, email, machineId, duration, days } = req.body || {};
  if (!pluginSlug || !name || !email || !duration) {
    res.status(400).json({ error: 'pluginSlug, name, email, and duration are required' });
    return;
  }

  const { rows: pluginRows } = await sql`SELECT id, name FROM plugins WHERE slug = ${pluginSlug}`;
  const plugin = pluginRows[0];
  if (!plugin) {
    res.status(404).json({ error: `No plugin with slug "${pluginSlug}"` });
    return;
  }

  const envVar = envVarNameForSlug(pluginSlug);
  const privateKey = process.env[envVar];
  if (!privateKey) {
    res.status(400).json({
      error: `No server-side signing key configured for ${plugin.name}. `
        + `Set the ${envVar} environment variable to that plugin's private key `
        + `(the same value used locally in Licensing/keys/private.key), or use the `
        + `local CLI + "paste key" flow instead for this plugin.`,
    });
    return;
  }

  let expiresAt;
  try {
    expiresAt = resolveExpiry(duration, days);
  } catch (err) {
    res.status(400).json({ error: err.message });
    return;
  }

  let keyBlob;
  try {
    keyBlob = signLicense({ name, email, machineId: machineId || '', expiresAt }, privateKey);
  } catch (err) {
    res.status(500).json({ error: `Signing failed: ${err.message}` });
    return;
  }

  const { rows: subRows } = await sql`
    INSERT INTO subscribers (email, name) VALUES (${email}, ${name})
    ON CONFLICT (email) DO UPDATE SET name = COALESCE(EXCLUDED.name, subscribers.name)
    RETURNING id
  `;

  const { rows } = await sql`
    INSERT INTO license_tokens (subscriber_id, plugin_id, key_blob, machine_id)
    VALUES (${subRows[0].id}, ${plugin.id}, ${keyBlob}, ${machineId || null})
    RETURNING *
  `;

  res.status(201).json({ ...rows[0], key_blob: keyBlob });
};
