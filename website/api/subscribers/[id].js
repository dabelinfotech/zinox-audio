const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');

module.exports = async (req, res) => {
  if (req.method !== 'DELETE') {
    res.status(405).json({ error: 'Method not allowed' });
    return;
  }
  if (!requireAdmin(req, res)) return;

  const id = Number(req.query.id);
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
