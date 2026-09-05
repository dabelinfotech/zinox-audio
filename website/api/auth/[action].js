// Merges login.js + logout.js + status.js into one function, purely to make
// room in Vercel Hobby's 12-function budget for splitting the broken
// [[...id]].js catch-alls below back into real index.js + [id].js pairs.
// This one is a single, MANDATORY dynamic segment ([action], one bracket) -
// unlike those, every URL that ever reaches this file always has exactly
// one segment (login/logout/status), so there's no zero-segment case to
// get wrong. /api/auth/login, /api/auth/logout, /api/auth/status all
// resolve here exactly as before; no frontend changes needed.
const { checkPassword, setSessionCookie, clearSessionCookie, isAuthenticated } = require('../_lib/auth');

module.exports = async (req, res) => {
  const action = Array.isArray(req.query.action) ? req.query.action[0] : req.query.action;

  if (action === 'login') {
    if (req.method !== 'POST') {
      res.status(405).json({ error: 'Method not allowed' });
      return;
    }
    const { password } = req.body || {};
    if (!checkPassword(password)) {
      // Same message either way - don't reveal whether ADMIN_PASSWORD is unset.
      res.status(401).json({ error: 'Incorrect password' });
      return;
    }
    setSessionCookie(res);
    res.status(200).json({ ok: true });
    return;
  }

  if (action === 'logout') {
    clearSessionCookie(res);
    res.status(200).json({ ok: true });
    return;
  }

  if (action === 'status') {
    res.status(200).json({ authenticated: isAuthenticated(req) });
    return;
  }

  res.status(404).json({ error: 'Not found' });
};
