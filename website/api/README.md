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
| `api/auth/[action].js` | Login/logout/status for the single admin session (signed cookie, no user table) - one file, dispatching on the `login`/`logout`/`status` segment |
| `api/plugins/index.js`, `[id].js` | CRUD for the plugin listing (name, description, pricing) |
| `api/files/index.js`, `[id].js`, `upload-token.js` | List/delete installers, plus the client → Blob direct-upload handshake (its own function, unaffected by anything below) |
| `api/subscribers/index.js`, `[id].js` | Public POST for trial signups; admin GET/DELETE to manage the list |
| `api/tokens/index.js`, `[id].js` | Records license keys issued via the offline `HeliumLicenseGen`/`ZinoxLicenseGen` CLI tools - **the signing private key never touches this server**, this only logs the already-signed key blob for your own reference, and lets you mark one revoked |
| `api/tokens/issue.js` | Optional alternative: signs *and* logs a key in one request, from the admin panel, no CLI needed - only if that plugin has a `LICENSE_PRIVATE_KEY_<SLUG>` env var set (see "Server-side key signing" below). This one **does** put the private key on this server. |
| `api/downloads/index.js` | Two things in one function, told apart by query shape: admin-gated usage log (no `slug`/`platform`), or the public `GET /api/downloads?slug=helium&platform=windows` hit-log-and-redirect (was `/api/download`, singular, before the routing fix below - nothing linked to it yet, so nothing broke) |

### Why `index.js` + `[id].js` instead of one `[[...id]].js` per resource

Every one of `plugins`, `subscribers`, `files`, and `tokens` used to be a
single `[[...id]].js` file - Next.js's "optional catch-all" convention,
meant to match both the bare path and any `/:id` under it in one function,
to stay under Vercel Hobby's 12-function limit. **That convention doesn't
work outside Next.js.** Plain Vercel Serverless Functions only support
`[...id].js` (catch-all, *one or more* segments) - there's no zero-segment
match, so `/api/plugins`, `/api/tokens`, `/api/subscribers`, and `/api/files`
(no id) all silently 404'd, while `/api/plugins/1` etc. worked fine. That's
what was breaking the Plugin dropdown (empty - the list call 404'd) and
"Log key" (404 on `POST /api/tokens`).

The fix is back to two files per resource - the standard, unambiguous way -
which costs 4 extra functions (8 instead of 4). To stay at exactly 12, two
unrelated pairs were merged into one function each: the three tiny
`auth/*` files into `auth/[action].js`, and `download.js` into
`downloads/index.js` (told apart by query shape, see the table above).
Both of those are genuine single-segment routes or method/shape
dispatches, not an attempt at the same broken optional-catch-all trick.

## Server-side key signing (optional, per plugin)

By default, issuing a license key means running that plugin's `LicenseKeyGen`
CLI tool locally and pasting the result into the admin panel - the private
key never leaves whoever's machine runs the tool. `api/tokens/issue.js` is
an opt-in alternative that signs the key here instead, so the admin panel
can issue one from any browser with no local tool at all. Both paths
produce cryptographically identical keys and can be used interchangeably
for the same plugin.

**This is a real trade-off, not a config detail**: turning it on for a
plugin means that plugin's private key lives in a Vercel environment
variable. If this project or function is ever compromised, every key for
that plugin - past and future - becomes forgeable, and there's no cheap
fix: rotating the key invalidates every key already issued to customers and
requires re-shipping the plugin with the new public key. Only turn this on
if you're comfortable with that.

To enable it for a plugin, set an environment variable named
`LICENSE_PRIVATE_KEY_<SLUG>` (the plugin's slug, uppercased, `-` → `_` -
e.g. `zinox-vocals` → `LICENSE_PRIVATE_KEY_ZINOX_VOCALS`) to the exact
contents of that plugin's `Licensing/keys/private.key` file, then redeploy.
Leave it unset for any plugin you want to keep local-CLI-only - the "Log
key" paste-in flow keeps working regardless, for every plugin, always.

## Things I couldn't verify without a live deployment

- The `index.js` + `[id].js` split (plugins, subscribers, files, tokens)
  and the `auth/[action].js` / `downloads/index.js` merges are ordinary,
  well-understood filesystem routing - no double-bracket tricks anywhere
  in this batch - but I don't have a Vercel account to deploy and hit
  these live. After deploying, a quick check worth doing: the Plugin
  dropdown in the admin panel populates, "Log key" no longer 404s, and
  `curl https://<domain>/api/plugins` returns JSON instead of 404.
- `api/tokens/issue.js`'s actual RSA signing (hashing, modular exponentiation,
  XML/Base64 packaging in `api/_lib/licensing.js`) was tested end-to-end
  locally with Node against the real Zinox Vocals private key, and the
  resulting keys verified successfully against the compiled plugin - that
  part is solid. `/api/tokens/issue` reaching this file rather than the
  sibling `tokens/[id].js` rests on the same precedent as
  `files/upload-token.js` already coexisting with `files/[id].js` - a
  literal path always wins over a same-level dynamic one - but confirm
  this the first time you use it (a distinct 400/500 from *this* file,
  not "Invalid token id" from `[id].js`).
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
