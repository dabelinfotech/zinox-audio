# Project Context — Zinox Vocals / ZinoxAudio

Running summary of work across multiple sessions on the ZinoxAudio site,
admin backend, and Zinox Vocals' licensing system. Update this file rather
than replacing it wholesale when picking work back up.

## Repo

- GitHub: **https://github.com/dabelinfotech/zinox-audio** (renamed from
  `zinox-vocals` on 2026-09-04; GitHub auto-redirects the old URL).
- Local clone: `C:\Users\opeye\ZinoxVocals` (directory name unchanged —
  only the GitHub repo and its remote URL were renamed).
- Site source lives in `website/` and is deployed via Vercel at
  `zinox-audio.vercel.app`.
- **Heads up**: a *different, parallel* Claude Code session has also been
  pushing to this repo (it built Zinox Lacquer, Helium, and the whole
  admin backend while this session worked on licensing). Always
  `git fetch origin` and check `git log --oneline main..origin/main`
  before pushing — don't assume this branch is only ever moved by one
  session.

## Phase 1 — ZinoxAudio site rebrand (2026-09-04/05)

1. **Site identity → ZinoxAudio.** Was a single-product "Zinox Vocals"
   page; repositioned as the ZinoxAudio company site with Zinox Vocals as
   its first plugin.
2. **"Manual" nav link → "Plugins" dropdown** (`<details>`/`<summary>`,
   no framework) so future plugins just add another `<a>` to the same menu.
3. **Plugin docs consolidated into `website/plugins.html`**, migrated from
   an untracked, better-designed draft; old `manual.html` removed.
4. **`plugins.html` bugs fixed**: missing `<!DOCTYPE>`/charset (mojibake),
   a CSS Grid bug where `::before` counted as a grid item and collapsed
   every install-step's body text to one word per line, and an ad-hoc
   inline-style list variant replaced with a `.steps.flat` class.
5. **`plugins.html` switched to permanent dark theme** (was light-default
   with a dark opt-in).
6. **Plugin screenshot added** to the docs hero (`assets/zinox-vocals-plugin.png`).

Verified throughout by screenshotting rendered pages with headless Chrome
(`chrome.exe --headless=new --screenshot`) against a local
`python -m http.server` — no Playwright/`chromium-cli` available in this
environment. This is what actually caught the mojibake and grid bugs, not
reading the CSS.

**Open, not acted on**: the live `#features` section repeats a lot of the
"signal chain" rail directly above it, and its 7-card grid (4 cols) leaves
an uneven last row.

## Phase 2 — Subscription licensing (2026-09-05)

Full architecture and rationale in memory (`zinox_licensing_architecture` —
see the assistant's persistent memory if picking this up in a fresh
session; the short version:

- `Source/Licensing.cpp`/`.h` (verifier) + `Source/Tools/LicenseKeyGen.cpp`
  (offline CLI issuer) do RSA-signed, no-phone-home license keys.
- Added signed **expiry** support: `issue "Name" "email" (--monthly|
  --yearly|--days N|--perpetual) [--machine <id>]`. Expired-but-valid-
  signature is a distinct `Info::expired` state from a truly invalid key,
  surfaced distinctly in the plugin UI ("SUBSCRIPTION EXPIRED" vs "TRIAL
  EXPIRED"). Anti-clock-rollback via the same high-water-mark trick the
  trial clock already used.
- **Generated the real, permanent key pair** — private key at
  `Licensing/keys/private.key` (gitignored, never commit), public key
  embedded in `Source/Licensing.cpp`'s `kPublicKey` and pushed. Was an
  empty fail-closed placeholder before this.
- Rebuilt Standalone + VST3 with the real key; regenerated the Windows
  installer (`Installer/ZinoxVocals.iss` → Inno Setup, installed via
  `winget install JRSoftware.InnoSetup`) and swapped it into
  `website/downloads/` + `website/index.html`'s download links.
  **macOS `.pkg` still has the old placeholder key baked in** — no Mac
  toolchain available here to rebuild it.
- **Server-side signing added alongside the CLI**, not replacing it:
  `website/api/tokens/issue.js` + `website/api/_lib/licensing.js`
  reimplement JUCE's exact raw/textbook RSA signing in Node (NOT
  `crypto.sign()` — incompatible padding). Opt-in per plugin via a
  `LICENSE_PRIVATE_KEY_<SLUG>` Vercel env var; unset, that plugin stays
  CLI-only. Verified by signing real test keys and checking them against
  the actual compiled plugin binary (monthly/yearly/perpetual/machine-
  locked/special-XML-characters all confirmed correct).
- Admin panel Tokens tab got a command-generator helper (plugin/name/
  email/duration/machine → exact CLI command to copy) plus the
  "Generate on server" button wired to the new endpoint.

**User decisions along the way** (don't re-litigate without reason): keep
both local-CLI and server-side signing available side by side, rather than
picking one; only Zinox Vocals has a server-side key configured so far.

## Phase 3 — Discovering & merging parallel work (2026-09-05)

Mid-Phase-2, discovered the other session's 11 commits already on
`origin/main`: Zinox Lacquer + Helium plugin pages, and a full admin
backend (Vercel Postgres + Blob) — `website/admin/index.html`,
`website/api/*`, `website/sql/schema.sql`. Diffed and trial-merged
(`--no-commit --no-ff`) before touching anything — conflict-free, since the
two efforts touched disjoint files. Merged locally, then kept building on
top. See `multi_session_collab_risk` in memory for why this check matters
every time on this repo specifically.

`ADMIN_PASSWORD`/`SESSION_SECRET` were generated and handed to the user to
set in Vercel (values not written here — check Vercel's env vars directly,
or the password manager the user was told to store them in).

## Phase 4 — Fixed the admin API's 404s (2026-09-05)

`plugins`, `tokens`, `subscribers`, `files` were each a single
`[[...id]].js` (Next.js's "optional catch-all" convention) — but **plain
Vercel Serverless Functions don't support that**, only the mandatory
`[...id].js` (one-or-more segments). The bare paths (`/api/plugins`,
`/api/tokens`, no id) silently 404'd — this is what broke the admin
panel's Plugin dropdown and the "Log key" button.

Fixed by splitting each back into `index.js` + `[id].js` (standard,
unambiguous filesystem routing). That's +4 functions against Vercel
Hobby's 12-function cap, so two unrelated pairs were consolidated to
compensate: `auth/login.js`+`logout.js`+`status.js` → one
`auth/[action].js` (a genuine single-segment route, not the broken
pattern), and `download.js` → merged into `downloads/index.js` (told
apart by query shape; moves the public redirect from `/api/download` to
`/api/downloads`, but nothing was linked to the old path yet). Landed at
exactly 12 functions. No admin-panel-facing URLs changed except that one
unlinked public redirect path.

**Not verified live** — no Vercel account/CLI available in this dev
environment. After deploying, confirm: Plugin dropdown populates, "Log
key" no longer 404s, `curl https://zinox-audio.vercel.app/api/plugins`
returns JSON.

## Phase 5 — High band DSP tuning (2026-09-05)

`Source/DSP/ToneStack.h`: the High band (all three voicings — AIR shelf,
BRIGHT shelf, PRESENCE bell) now applies **1.4× the knob's dB reading** to
the actual filter gain (`dbToGain(highGain * 1.4f)`), so the knob's ±18 dB
range/display/automation are unchanged but the audible effect per dB moved
is 40% stronger. Rebuilt Standalone/VST3 and the Windows installer again
(same v1.1.0 filename — a tuning change, not a version bump; flagged to the
user that a real version bump would need updating that filename/downloads
links/docs consistently if wanted later).

## Other repos referenced from this session

- **`dabelinfotech/dabeltech-web`** (separate business, separate repo,
  cloned to `E:\dabeltech-web`) — was missing `DATABASE_URL` entirely
  (`ECONNREFUSED 127.0.0.1:5432`, i.e. `pg`'s Pool falling back to
  localhost because the env var was never set). Generated
  `ADMIN_PASSWORD`/`SESSION_SECRET` for it; **actually provisioning a
  Postgres database is still on the user** — not completable without his
  Vercel/Neon/Supabase account.
- **`E:\ZinoxAudio-Helium`** — Helium's own plugin repo, referenced only to
  confirm JUCE source (e.g. `RSAKey`/`Time` implementations) since the
  vendored JUCE checkout there was easier to search than re-fetching it
  here.
