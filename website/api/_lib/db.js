// Thin re-export so every API route imports the database from one place -
// makes it a one-line change later if the underlying provider ever changes.
// `sql` is a tagged-template query function: sql`select * from x where id=${id}`,
// which parameterizes the value automatically (safe against SQL injection).
const { sql } = require('@vercel/postgres');

module.exports = { sql };
