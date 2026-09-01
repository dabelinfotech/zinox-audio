// Merged from the former tokens/index.js + tokens/[id].js into one optional
// catch-all route, purely to stay under Vercel Hobby's 12 serverless
// function limit - /api/tokens and /api/tokens/:id both still resolve here
// exactly as before, no frontend changes needed.
const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');

module.exports = async (req, res) => {
  if (!requireAdmin(req, res)) return;

  const idParam = Array.isArray(req.query.id) ? req.query.id[0] : req.query.id;

  if (idParam === undefined) {
    // /api/tokens
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
      // ZinoxLicenseGen / ZinoxLacquerLicenseGen (see Source/Tools/LicenseKeyGen.cpp) -
      // this endpoint never signs anything itself, the private key never touches this server.
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
    return;
  }

  // /api/tokens/:id
  const id = Number(idParam);
  if (!Number.isInteger(id)) {
    res.status(400).json({ error: 'Invalid token id' });
    return;
  }

  if (req.method === 'PATCH') {
    const { revoked } = req.body || {};
    const { rows } = await sql`
      UPDATE license_tokens SET revoked = ${!!revoked} WHERE id = ${id} RETURNING *
    `;
    if (!rows[0]) {
      res.status(404).json({ error: 'Not found' });
      return;
    }
    res.status(200).json(rows[0]);
    return;
  }

  if (req.method === 'DELETE') {
    await sql`DELETE FROM license_tokens WHERE id = ${id}`;
    res.status(204).end();
    return;
  }

  res.status(405).json({ error: 'Method not allowed' });
};
