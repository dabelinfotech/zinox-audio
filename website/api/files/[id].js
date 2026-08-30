const { del } = require('@vercel/blob');
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
    res.status(400).json({ error: 'Invalid file id' });
    return;
  }

  const { rows } = await sql`SELECT blob_url FROM plugin_files WHERE id = ${id}`;
  if (!rows[0]) {
    res.status(404).json({ error: 'Not found' });
    return;
  }

  await del(rows[0].blob_url).catch(() => {
    // Blob already gone (or URL malformed) - still remove the DB row below
    // rather than leaving an orphaned record the admin can't get rid of.
  });
  await sql`DELETE FROM plugin_files WHERE id = ${id}`;

  res.status(204).end();
};
