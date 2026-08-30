const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');

module.exports = async (req, res) => {
  if (!requireAdmin(req, res)) return;

  if (req.method === 'GET') {
    const { rows } = await sql`
      SELECT t.*, s.email AS subscriber_email, s.name AS subscriber_name, p.slug AS plugin_slug, p.name AS plugin_name
      FROM license_tokens t
      JOIN plugins p ON p.id = t.plugin_id
      LEFT JOIN subscribers s ON s.id = t.subscriber_id
      ORDER BY t.issued_at DESC
    `;
    res.status(200).json(rows);
    return;
  }

  if (req.method === 'POST') {
    // Logs a key that was already signed offline via HeliumLicenseGen /
    // ZinoxLicenseGen (see Source/Tools/LicenseKeyGen.cpp) - this endpoint
    // never signs anything itself, the private key never touches this server.
    const { subscriberEmail, pluginSlug, keyBlob, machineId } = req.body || {};
    if (!subscriberEmail || !pluginSlug || !keyBlob) {
      res.status(400).json({ error: 'subscriberEmail, pluginSlug, and keyBlob are required' });
      return;
    }

    const { rows: pluginRows } = await sql`SELECT id FROM plugins WHERE slug = ${pluginSlug}`;
    if (!pluginRows[0]) {
      res.status(404).json({ error: `No plugin with slug "${pluginSlug}"` });
      return;
    }

    const { rows: subRows } = await sql`
      INSERT INTO subscribers (email) VALUES (${subscriberEmail})
      ON CONFLICT (email) DO UPDATE SET email = EXCLUDED.email
      RETURNING id
    `;

    const { rows } = await sql`
      INSERT INTO license_tokens (subscriber_id, plugin_id, key_blob, machine_id)
      VALUES (${subRows[0].id}, ${pluginRows[0].id}, ${keyBlob}, ${machineId || null})
      RETURNING *
    `;
    res.status(201).json(rows[0]);
    return;
  }

  res.status(405).json({ error: 'Method not allowed' });
};
