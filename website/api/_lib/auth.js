// Minimal signed-session auth for a single admin account - no user table,
// no third-party auth provider. The admin password lives in the
// ADMIN_PASSWORD env var; a session is a small JSON payload signed with
// HMAC-SHA256 (SESSION_SECRET env var) so the server never needs to store
// session state anywhere.
//
// This is deliberately simple because there is exactly one admin. If you
// ever need more than one person with access, replace this with real
// per-user accounts before relying on it.
const crypto = require('crypto');

const COOKIE_NAME = 'zx_admin_session';
const SESSION_TTL_MS = 12 * 60 * 60 * 1000; // 12 hours

function sign(payload) {
  const secret = process.env.SESSION_SECRET;
  if (!secret) throw new Error('SESSION_SECRET is not set');
  const body = Buffer.from(JSON.stringify(payload)).toString('base64url');
  const sig = crypto.createHmac('sha256', secret).update(body).digest('base64url');
  return `${body}.${sig}`;
}

function verify(token) {
  const secret = process.env.SESSION_SECRET;
  if (!secret || !token) return null;
  const [body, sig] = token.split('.');
  if (!body || !sig) return null;

  const expectedSig = crypto.createHmac('sha256', secret).update(body).digest('base64url');
  const sigBuf = Buffer.from(sig);
  const expectedBuf = Buffer.from(expectedSig);
  if (sigBuf.length !== expectedBuf.length || !crypto.timingSafeEqual(sigBuf, expectedBuf)) {
    return null;
  }

  try {
    const payload = JSON.parse(Buffer.from(body, 'base64url').toString('utf8'));
    if (typeof payload.exp !== 'number' || Date.now() > payload.exp) return null;
    return payload;
  } catch {
    return null;
  }
}

function parseCookies(req) {
  const header = req.headers.cookie;
  const out = {};
  if (!header) return out;
  for (const part of header.split(';')) {
    const idx = part.indexOf('=');
    if (idx === -1) continue;
    out[part.slice(0, idx).trim()] = decodeURIComponent(part.slice(idx + 1).trim());
  }
  return out;
}

function setSessionCookie(res) {
  const token = sign({ exp: Date.now() + SESSION_TTL_MS });
  res.setHeader(
    'Set-Cookie',
    `${COOKIE_NAME}=${token}; HttpOnly; Secure; SameSite=Strict; Path=/; Max-Age=${Math.floor(SESSION_TTL_MS / 1000)}`
  );
}

function clearSessionCookie(res) {
  res.setHeader('Set-Cookie', `${COOKIE_NAME}=; HttpOnly; Secure; SameSite=Strict; Path=/; Max-Age=0`);
}

function isAuthenticated(req) {
  const cookies = parseCookies(req);
  return verify(cookies[COOKIE_NAME]) !== null;
}

/** Call at the top of any admin-only handler. Sends 401 and returns false
 *  if the caller isn't logged in - the handler should just `return` when
 *  this returns false. */
function requireAdmin(req, res) {
  if (!isAuthenticated(req)) {
    res.status(401).json({ error: 'Not authenticated' });
    return false;
  }
  return true;
}

function checkPassword(candidate) {
  const expected = process.env.ADMIN_PASSWORD;
  if (!expected || !candidate) return false;
  const a = Buffer.from(candidate);
  const b = Buffer.from(expected);
  return a.length === b.length && crypto.timingSafeEqual(a, b);
}

function hashIp(ip) {
  const secret = process.env.SESSION_SECRET || 'fallback';
  return crypto.createHmac('sha256', secret).update(ip || 'unknown').digest('hex').slice(0, 32);
}

module.exports = {
  isAuthenticated,
  requireAdmin,
  checkPassword,
  setSessionCookie,
  clearSessionCookie,
  hashIp,
};
