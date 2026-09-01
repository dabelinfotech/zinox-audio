// Merged from the former subscribers/index.js + subscribers/[id].js into one
// optional catch-all route, purely to stay under Vercel Hobby's 12
// serverless function limit - /api/subscribers and /api/subscribers/:id both
// still resolve here exactly as before, no frontend changes needed.
const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');
const { allowCrossOrigin } = require('../_lib/cors');

const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

module.exports = async (req, res) => {
  const idParam = Array.isArray(req.query.id) ? req.query.id[0] : req.query.id;

  if (idParam === undefined) {
    // /api/subscribers
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
    return;
  }

  // /api/subscribers/:id
  if (req.method !== 'DELETE') {
    res.status(405).json({ error: 'Method not allowed' });
    return;
  }
  if (!requireAdmin(req, res)) return;

  const id = Number(idParam);
  if (!Number.isInteger(id)) {
    res.status(400).json({ error: 'Invalid subscriber id' });
    return;
  }

  // license_tokens.subscriber_id and download_events.subscriber_id are both
  // ON DELETE SET NULL, so this un-links their history rather than deleting
  // it - the token/download records themselves stay for the audit trail.
  await sql`DELETE FROM subscribers WHERE id = ${id}`;
  res.status(204).end();
};
