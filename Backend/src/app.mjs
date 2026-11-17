import { createServer } from 'node:http';
import { timingSafeEqual } from 'node:crypto';
import { ApiError, validateCharacterId } from './store.mjs';

function send(response, status, data, headers = {}) {
  response.writeHead(status, { 'Content-Type': 'application/json; charset=utf-8',
    'Cache-Control': 'no-store', 'X-Content-Type-Options': 'nosniff', ...headers });
  response.end(JSON.stringify(data));
}

function readJson(request) {
  if (request.headers['content-type']?.split(';')[0].trim().toLowerCase() !== 'application/json') {
    request.resume();
    return Promise.reject(new ApiError(415, 'json_required'));
  }
  return new Promise((resolve, reject) => {
    let size = 0;
    const chunks = [];
    request.on('data', chunk => {
      size += chunk.length;
      if (size > 8192) { reject(new ApiError(413, 'body_too_large')); return; }
      chunks.push(chunk);
    });
    request.on('end', () => {
      if (size > 8192) return;
      try { resolve(JSON.parse(Buffer.concat(chunks).toString('utf8'))); }
      catch { reject(new ApiError(400, 'invalid_json')); }
    });
    request.on('error', reject);
    request.on('aborted', () => reject(new ApiError(400, 'request_aborted')));
  });
}

export function createApp({ store, token, logError = () => {} }) {
  if (typeof token !== 'string' || token.length < 32 || token.startsWith('replace-with-')) {
    throw new Error('EXPERIENCE_API_TOKEN must be a private random token of at least 32 characters');
  }
  const expected = Buffer.from(`Bearer ${token}`);
  const server = createServer(async (request, response) => {
    try {
      const url = new URL(request.url, 'http://localhost');
      if (request.method === 'GET' && url.pathname === '/health') {
        send(response, 200, { status: 'ok' }); return;
      }
      const supplied = Buffer.from(request.headers.authorization ?? '');
      if (supplied.length !== expected.length || !timingSafeEqual(supplied, expected)) {
        request.resume(); send(response, 401, { error: 'unauthorized' }); return;
      }
      const match = /^\/v1\/characters\/([^/]+)\/experience$/.exec(url.pathname);
      if (!match) { request.resume(); send(response, 404, { error: 'route_not_found' }); return; }
      let characterId;
      try { characterId = decodeURIComponent(match[1]); } catch { throw new ApiError(400, 'invalid_character_id'); }
      validateCharacterId(characterId);
      if (request.method === 'GET') {
        send(response, 200, store.get(characterId)); return;
      }
      if (request.method === 'PUT') {
        const body = await readJson(request);
        send(response, 200, store.save(characterId, body)); return;
      }
      request.resume(); send(response, 405, { error: 'method_not_allowed' }, { Allow: 'GET, PUT' });
    } catch (error) {
      if (response.destroyed || response.writableEnded) return;
      if (error instanceof ApiError) send(response, error.status, { error: error.code, ...error.details });
      else { logError(error); send(response, 500, { error: 'internal_error' }); }
    }
  });
  server.requestTimeout = 15_000;
  server.headersTimeout = 10_000;
  server.maxHeadersCount = 32;
  return server;
}
