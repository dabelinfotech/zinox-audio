// Split back out of the former subscribers/[[...id]].js optional catch-all,
// which silently 404'd at exactly this path: Vercel's plain (non-Next.js)
// Serverless Functions don't support the double-bracket "optional
// catch-all" convention for matching zero path segments, only Next.js
// does. See subscribers/[id].js for the id-suffixed route, and
// website/api/README.md for the full story.
const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');
const { allowCrossOrigin } = require('../_lib/cors');

const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

module.exports = async (req, res) => {
  if (allowCrossOrigin(req, res)) return;

  if (req.method === 'POST') {
    // Public: trial-signup forms call this directly (in addition to, or
    // instead of, Web3Forms) so signups end up queryable in the admin panel.
    const { name, email } = req.body || {};
    if (!email || !EMAIL_RE.test(email)) {
      res.status(400).json({ error: 'A valid email is required' });
      return;
    }
    const { rows } = await sql`
      INSERT INTO subscribers (name, email)
      VALUES (${name || null}, ${email})
      ON CONFLICT (email) DO UPDATE SET name = COALESCE(EXCLUDED.name, subscribers.name)
      RETURNING *
    `;
    res.status(201).json(rows[0]);
    return;
  }

  if (req.method === 'GET') {
    if (!requireAdmin(req, res)) return;
    const { rows } = await sql`SELECT * FROM subscribers ORDER BY created_at DESC`;
    res.status(200).json(rows);
    return;
  }

  res.status(405).json({ error: 'Method not allowed' });
};
