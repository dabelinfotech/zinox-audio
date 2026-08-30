# ZinoxAudio admin backend

Serverless functions (Vercel Node.js runtime) behind `/admin` - upload/delete/
update plugins and installer files, and view subscribers, license tokens,
and download/usage records. Everything else on the site is still the plain
static HTML it was before; this is additive.

## One-time setup (do this before the admin panel will work)

1. **Enable Vercel Postgres.** In the Vercel dashboard → this project →
   Storage → Create Database → Postgres → Connect to this project. This
   auto-injects `POSTGRES_URL` (and friends) as environment variables - you
   don't set those by hand.

2. **Enable Vercel Blob.** Storage → Create → Blob → Connect to this
   project. This auto-injects `BLOB_READ_WRITE_TOKEN`.

3. **Run the schema once.** Storage → your Postgres database → Query, paste
   the contents of [`website/sql/schema.sql`](../sql/schema.sql), run it.
   Safe to re-run later (every statement is idempotent) if you add fields.

4. **Set two more environment variables** (Project Settings → Environment
   Variables):
   - `ADMIN_PASSWORD` - whatever password you want to log into `/admin`
     with. There's only one admin account; there's no username.
   - `SESSION_SECRET` - a long random string, e.g. generate one with
     `openssl rand -hex 32`. Signs the admin session cookie.

5. **Redeploy** so the new env vars and dependencies (`@vercel/blob`,
   `@vercel/postgres` in `website/package.json`) take effect.

6. Visit `https://<your-domain>/admin/` and log in with `ADMIN_PASSWORD`.

## What's where

| Path | What it does |
|---|---|
| `api/auth/*` | Login/logout/status for the single admin session (signed cookie, no user table) |
| `api/plugins/*` | CRUD for the plugin listing (name, description, pricing) |
| `api/files/*` | Upload (client → Blob directly, admin-only token), list, delete installers |
| `api/subscribers/*` | Public POST for trial signups; admin GET/DELETE to manage the list |
| `api/tokens/*` | Records license keys issued via the offline `HeliumLicenseGen`/`ZinoxLicenseGen` CLI tools - **the signing private key never touches this server**, this only logs the already-signed key blob for your own reference, and lets you mark one revoked |
| `api/downloads/*` | Read-only usage log - every hit on `/api/download` |
| `api/download.js` | Public: `GET /api/download?slug=helium&platform=windows` logs the hit and redirects to the current uploaded file. Additive - existing static `/downloads/*.exe` links keep working untouched until you choose to switch a button over to this |

## Things I couldn't verify without a live deployment

- `api/files/upload-token.js` implements `@vercel/blob`'s client-upload
  handshake (`handleUpload`/`onUploadCompleted`) from documentation, not
  from a live test run - I don't have a Vercel account to actually deploy
  and exercise this against. If uploads fail after setup, this is the
  first file to check against the current
  [`@vercel/blob` client-upload docs](https://vercel.com/docs/storage/vercel-blob/client-upload).
- `onUploadCompleted` (the part that writes the `plugin_files` row) is
  called by Vercel's Blob infrastructure over the network, which only
  reaches a real deployment - it will not fire against `vercel dev` on
  localhost. Test uploads on a deployed Preview/Production URL.
- The admin UI loads `@vercel/blob`'s browser client from `esm.sh` at
  `https://esm.sh/@vercel/blob@0.27.1/client` rather than bundling it, to
  avoid adding a build step to what's otherwise a plain static site. Pin
  that version to whatever's actually in `website/package.json` if you
  bump it.

## Security notes

- One admin account, password-based. Fine for a single operator; if this
  is ever shared with someone else, replace it with real per-user auth
  first.
- `ADMIN_PASSWORD` and `SESSION_SECRET` must only ever live as Vercel
  environment variables - never commit them.
- The download-log stores a salted hash of the requester's IP, not the raw
  address, to keep this consistent with `privacy.html`'s existing "no
  telemetry" promise (which is specifically about the plugin binary
  itself - this log is about website traffic, a normal thing for any
  server to record, just kept as anonymous as it can be while still being
  useful).
