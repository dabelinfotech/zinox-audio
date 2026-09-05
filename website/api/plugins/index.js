// Split back out of the former plugins/[[...id]].js optional catch-all,
// which silently 404'd at exactly this path: Vercel's plain (non-Next.js)
// Serverless Functions don't support the double-bracket "optional
// catch-all" convention for matching zero path segments, only Next.js
// does. See plugins/[id].js for the id-suffixed route, and
// website/api/README.md for the full story and the function-count budget
// this was split back out under.
const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');

module.exports = async (req, res) => {
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
};
