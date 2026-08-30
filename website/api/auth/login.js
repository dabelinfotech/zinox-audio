const { checkPassword, setSessionCookie } = require('../_lib/auth');

module.exports = async (req, res) => {
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
};
