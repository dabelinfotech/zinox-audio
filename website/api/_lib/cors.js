// Applied only to the handful of endpoints meant to be called from other
// ZinoxAudio product sites (e.g. Helium's own site, once deployed, posting
// a trial signup here). Admin endpoints do NOT use this - they rely on the
// SameSite=Strict session cookie instead, which only works same-origin.
function allowCrossOrigin(req, res) {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
  if (req.method === 'OPTIONS') {
    res.status(204).end();
    return true; // caller should return immediately
  }
  return false;
}

module.exports = { allowCrossOrigin };
