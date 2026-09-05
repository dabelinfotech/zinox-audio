const { sql } = require('../_lib/db');
const { requireAdmin } = require('../_lib/auth');

module.exports = async (req, res) => {
  if (req.method !== 'GET') {
    res.status(405).json({ error: 'Method not allowed' });
    return;
  }
  if (!requireAdmin(req, res)) return;

  const limit = Math.min(Number(req.query.limit) || 100, 500);
  const offset = Number(req.query.offset) || 0;

  const { rows } = await sql`
    SELECT d.*, p.slug AS plugin_slug, p.name AS plugin_name, s.email AS subscriber_email
    FROM download_events d
    JOIN plugins p ON p.id = d.plugin_id
    LEFT JOIN subscribers s ON s.id = d.subscriber_id
    ORDER BY d.downloaded_at DESC
    LIMIT ${limit} OFFSET ${offset}
  `;
  res.status(200).json(rows);
};
