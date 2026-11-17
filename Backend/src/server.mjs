import { resolve } from 'node:path';
import { ExperienceStore } from './store.mjs';
import { createApp } from './app.mjs';

const host = process.env.HOST ?? '127.0.0.1';
const port = Number(process.env.PORT ?? 3001);
if (!Number.isInteger(port) || port < 1 || port > 65535) throw new Error('Invalid PORT');
const store = new ExperienceStore(resolve(process.env.DATABASE_PATH ?? './data/experience.sqlite'));
let server;
try {
  server = createApp({ store, token: process.env.EXPERIENCE_API_TOKEN,
    logError: error => console.error('Experience database request failed:', error.name) });
} catch (error) { store.close(); throw error; }
server.on('error', error => { console.error('Experience service failed:', error.code); store.close(); process.exitCode = 1; });
server.listen(port, host, () => console.log(`Experience backend listening on http://${host}:${port}`));
let stopping = false;
function stop() {
  if (stopping) return;
  stopping = true;
  server.close(() => { store.close(); process.exitCode = 0; });
  server.closeIdleConnections();
  setTimeout(() => process.exit(1), 5000).unref();
}
process.on('SIGINT', stop);
process.on('SIGTERM', stop);
