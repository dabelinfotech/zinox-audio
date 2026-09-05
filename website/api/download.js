// Public endpoint: GET /api/download?slug=helium&platform=windows[&email=...]
// Looks up the current installer for that plugin/platform, logs the hit,
// and redirects the browser to the actual file on Blob storage.
//
// This is additive, not a replacement: existing static links under
// /downloads/*.exe|*.pkg keep working exactly as before. Point new download
// buttons at this endpoint once you've uploaded a file for that plugin
// through the admin panel - until then this returns 404 for it.
const { sql } = require('./_lib/db');
const { hashIp } = require('./_lib/auth');
const { allowCrossOrigin } = require('./_lib/cors');

module.exports = async (req, res) => {
  if (allowCrossOrigin(req, res)) return;
  if (req.method !== 'GET') {
    res.status(405).json({ error: 'Method not allowed' });
    return;
  }

  const { slug, platform, email } = req.query;
  if (!slug || !platform) {
    res.status(400).json({ error: 'slug and platform query params are required' });
    return;
  }

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
};
