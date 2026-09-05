// Split back out of the former plugins/[[...id]].js - see plugins/index.js
// for why, and for the base-path (/api/plugins, no id) route.
const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');

module.exports = async (req, res) => {
  const id = Number(req.query.id);
  if (!Number.isInteger(id)) {
    res.status(400).json({ error: 'Invalid plugin id' });
    return;
  }

  if (req.method === 'GET') {
    const { rows } = await sql`SELECT * FROM plugins WHERE id = ${id}`;
    if (!rows[0]) {
      res.status(404).json({ error: 'Not found' });
      return;
    }
    res.status(200).json(rows[0]);
    return;
  }

  if (req.method === 'PATCH') {
    if (!requireAdmin(req, res)) return;
    const { name, description, priceMonthlyCents, priceYearlyCents, screenshotUrl } = req.body || {};
    const { rows } = await sql`
      UPDATE plugins SET
        name = COALESCE(${name}, name),
        description = COALESCE(${description}, description),
        price_monthly_cents = COALESCE(${priceMonthlyCents}, price_monthly_cents),
        price_yearly_cents = COALESCE(${priceYearlyCents}, price_yearly_cents),
        screenshot_url = COALESCE(${screenshotUrl}, screenshot_url),
        updated_at = now()
      WHERE id = ${id}
      RETURNING *
    `;
    if (!rows[0]) {
      res.status(404).json({ error: 'Not found' });
      return;
    }
    res.status(200).json(rows[0]);
    return;
  }

  if (req.method === 'DELETE') {
    if (!requireAdmin(req, res)) return;
    // Cascades to plugin_files/license_tokens/download_events for this plugin
    // (see ON DELETE CASCADE in sql/schema.sql) - deleting a plugin here also
    // removes its file/token/download history, not just the listing.
    await sql`DELETE FROM plugins WHERE id = ${id}`;
    res.status(204).end();
    return;
  }

  res.status(405).json({ error: 'Method not allowed' });
};
