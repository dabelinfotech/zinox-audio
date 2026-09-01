// Merged from the former files/index.js + files/[id].js into one optional
// catch-all route, purely to stay under Vercel Hobby's 12 serverless
// function limit - /api/files and /api/files/:id both still resolve here
// exactly as before, no frontend changes needed. upload-token.js is left as
// its own function since it's a distinct, more complex upload handshake.
const { del } = require('@vercel/blob');
const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');

module.exports = async (req, res) => {
  const idParam = Array.isArray(req.query.id) ? req.query.id[0] : req.query.id;

  if (idParam === undefined) {
    // /api/files (optionally ?pluginId=)
    if (req.method !== 'GET') {
      res.status(405).json({ error: 'Method not allowed' });
      return;
    }
    if (!requireAdmin(req, res)) return;

    const pluginId = req.query.pluginId ? Number(req.query.pluginId) : null;
    const { rows } = pluginId
      ? await sql`
          SELECT f.*, p.slug AS plugin_slug, p.name AS plugin_name
          FROM plugin_files f JOIN plugins p ON p.id = f.plugin_id
          WHERE f.plugin_id = ${pluginId}
          ORDER BY f.uploaded_at DESC
        `
      : await sql`
          SELECT f.*, p.slug AS plugin_slug, p.name AS plugin_name
          FROM plugin_files f JOIN plugins p ON p.id = f.plugin_id
          ORDER BY f.uploaded_at DESC
        `;
    res.status(200).json(rows);
    return;
  }

  // /api/files/:id
  if (req.method !== 'DELETE') {
    res.status(405).json({ error: 'Method not allowed' });
    return;
  }
  if (!requireAdmin(req, res)) return;

  const id = Number(idParam);
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
