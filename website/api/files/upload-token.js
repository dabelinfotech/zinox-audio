// Handles the @vercel/blob client-upload handshake: the browser never sends
// the file bytes through this serverless function (which would hit
// Vercel's ~4.5MB request body limit long before a real installer's size),
// it uploads straight to Blob storage using a short-lived token that this
// endpoint issues after checking the admin session.
//
// NOTE: onUploadCompleted below is invoked by Vercel's Blob infrastructure
// calling back into this same route, which only works once this is
// deployed (Vercel Preview/Production) - it will NOT fire against a plain
// `vercel dev` / localhost server. Test the actual upload-then-DB-row flow
// on a deployed URL, not locally.
const { handleUpload } = require('@vercel/blob/client');
const { sql } = require('../_lib/db');
const { isAuthenticated } = require('../_lib/auth');

module.exports = async (req, res) => {
  if (req.method !== 'POST') {
    res.status(405).json({ error: 'Method not allowed' });
    return;
  }

  try {
    const jsonResponse = await handleUpload({
      body: req.body,
      request: req,
      onBeforeGenerateToken: async (pathname, clientPayload) => {
        if (!isAuthenticated(req)) {
          throw new Error('Not authenticated');
        }

        let meta = {};
        try {
          meta = JSON.parse(clientPayload || '{}');
        } catch {
          throw new Error('Invalid clientPayload');
        }
        if (!meta.pluginSlug || !meta.platform || !meta.version) {
          throw new Error('clientPayload must include pluginSlug, platform, version');
        }
        if (!['windows', 'macos'].includes(meta.platform)) {
          throw new Error('platform must be "windows" or "macos"');
        }

        return {
          allowedContentTypes: [
            'application/octet-stream',
            'application/x-msdownload',
            'application/x-newton-compatible-pkg',
            'application/vnd.microsoft.portable-executable',
          ],
          addRandomSuffix: false,
          tokenPayload: JSON.stringify(meta),
        };
      },
      onUploadCompleted: async ({ blob, tokenPayload }) => {
        const meta = JSON.parse(tokenPayload);

        const { rows: pluginRows } = await sql`SELECT id FROM plugins WHERE slug = ${meta.pluginSlug}`;
        if (!pluginRows[0]) return; // plugin was deleted between upload start and completion
        const pluginId = pluginRows[0].id;

        await sql`
          UPDATE plugin_files SET is_current = false
          WHERE plugin_id = ${pluginId} AND platform = ${meta.platform} AND is_current
        `;
        await sql`
          INSERT INTO plugin_files (plugin_id, platform, version, blob_url, size_bytes, is_current)
          VALUES (${pluginId}, ${meta.platform}, ${meta.version}, ${blob.url}, ${blob.size || null}, true)
        `;
      },
    });

    res.status(200).json(jsonResponse);
  } catch (err) {
    res.status(400).json({ error: err.message });
  }
};
