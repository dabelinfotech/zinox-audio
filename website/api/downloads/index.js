// Merges the former top-level download.js (public: logs a hit and redirects
// to the current installer) into this admin usage-log endpoint, purely to
// make room in Vercel Hobby's 12-function budget for splitting the broken
// [[...id]].js catch-alls elsewhere back into real index.js + [id].js pairs.
// Both are GET, so they're told apart by request shape: `slug`+`platform`
// present means the public redirect; otherwise, the admin-gated usage log.
//
// This moves the public endpoint from /api/download to /api/downloads -
// safe, because nothing links to /api/download yet (see its old comment:
// existing static /downloads/*.exe|*.pkg links are what buttons actually
// use today; this was additive and unlinked).
const { sql } = require('../_lib/db');
const { requireAdmin, hashIp } = require('../_lib/auth');
const { allowCrossOrigin } = require('../_lib/cors');

async function servePublicRedirect(req, res) {
  if (allowCrossOrigin(req, res)) return;

  const { slug, platform, email } = req.query;

  const { rows: pluginRows } = await sql`SELECT id FROM plugins WHERE slug = ${slug}`;
  if (!pluginRows[0]) {
    res.status(404).json({ error: `No plugin with slug "${slug}"` });
    return;
  }
  const pluginId = pluginRows[0].id;

  const { rows: fileRows } = await sql`
    SELECT blob_url FROM plugin_files
    WHERE plugin_id = ${pluginId} AND platform = ${platform} AND is_current
    ORDER BY uploaded_at DESC LIMIT 1
  `;
  if (!fileRows[0]) {
    res.status(404).json({ error: `No ${platform} file uploaded yet for "${slug}" - use the admin panel, or use the existing static download link for now` });
    return;
  }

  let subscriberId = null;
  if (email) {
    const { rows } = await sql`SELECT id FROM subscribers WHERE email = ${email}`;
    subscriberId = rows[0]?.id || null;
  }

  const forwardedFor = req.headers['x-forwarded-for'];
  const ip = Array.isArray(forwardedFor) ? forwardedFor[0] : (forwardedFor || '').split(',')[0].trim();

  await sql`
    INSERT INTO download_events (plugin_id, platform, subscriber_id, ip_hash)
    VALUES (${pluginId}, ${platform}, ${subscriberId}, ${hashIp(ip)})
  `;

  res.setHeader('Location', fileRows[0].blob_url);
  res.status(302).end();
}

async function serveUsageLog(req, res) {
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
}

module.exports = async (req, res) => {
  if (req.method !== 'GET') {
    res.status(405).json({ error: 'Method not allowed' });
    return;
  }

  if (req.query.slug && req.query.platform) {
    await servePublicRedirect(req, res);
  } else {
    await serveUsageLog(req, res);
  }
};
