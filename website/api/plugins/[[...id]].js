// Merged from the former plugins/index.js + plugins/[id].js into one
// optional catch-all route, purely to stay under Vercel Hobby's 12
// serverless function limit - /api/plugins and /api/plugins/:id both still
// resolve here exactly as before, no frontend changes needed.
const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');

module.exports = async (req, res) => {
  const idParam = Array.isArray(req.query.id) ? req.query.id[0] : req.query.id;

  if (idParam === undefined) {
    // /api/plugins
    if (req.method === 'GET') {
      // Public - the storefront pages can read this list; nothing sensitive in it.
      const { rows } = await sql`SELECT * FROM plugins ORDER BY name`;
      res.status(200).json(rows);
      return;
    }

    if (req.method === 'POST') {
      if (!requireAdmin(req, res)) return;
      const { slug, name, description, priceMonthlyCents, priceYearlyCents, screenshotUrl } = req.body || {};
      if (!slug || !name) {
        res.status(400).json({ error: 'slug and name are required' });
        return;
      }
      try {
        const { rows } = await sql`
          INSERT INTO plugins (slug, name, description, price_monthly_cents, price_yearly_cents, screenshot_url)
          VALUES (${slug}, ${name}, ${description || null}, ${priceMonthlyCents || null}, ${priceYearlyCents || null}, ${screenshotUrl || null})
          RETURNING *
        `;
        res.status(201).json(rows[0]);
      } catch (err) {
        if (String(err.message).includes('duplicate key')) {
          res.status(409).json({ error: `A plugin with slug "${slug}" already exists` });
        } else {
          res.status(500).json({ error: 'Could not create plugin' });
        }
      }
      return;
    }

    res.status(405).json({ error: 'Method not allowed' });
    return;
  }

  // /api/plugins/:id
  const id = Number(idParam);
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
