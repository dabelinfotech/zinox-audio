// Split back out of the former tokens/[[...id]].js - see tokens/index.js
// for why, and for the base-path route. This only ever matches a numeric
// id (Vercel resolves /api/tokens/issue to tokens/issue.js first, since a
// literal file always wins over a same-level dynamic route).
const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');

module.exports = async (req, res) => {
  if (!requireAdmin(req, res)) return;

  const id = Number(req.query.id);
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
