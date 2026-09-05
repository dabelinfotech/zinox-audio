// Split back out of the former files/[[...id]].js optional catch-all, which
// silently 404'd at exactly this path: Vercel's plain (non-Next.js)
// Serverless Functions don't support the double-bracket "optional
// catch-all" convention for matching zero path segments, only Next.js
// does. See files/[id].js for the id-suffixed route, and
// website/api/README.md for the full story. upload-token.js stays its own
// function, unaffected by any of this.
const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');

module.exports = async (req, res) => {
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
};
