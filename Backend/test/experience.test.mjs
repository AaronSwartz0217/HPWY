import { test } from 'node:test';
import assert from 'node:assert/strict';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { ExperienceStore, MAX_EXPERIENCE } from '../src/store.mjs';
import { createApp } from '../src/app.mjs';

const token = 'test-only-token-'.repeat(4);
async function fixture(t, path = ':memory:') {
  const store = new ExperienceStore(path);
  const server = createApp({ store, token });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  t.after(async () => {
    await new Promise(resolve => server.close(resolve));
    store.close();
  });
  const base = `http://127.0.0.1:${server.address().port}`;
  const api = async (method, id = 'hero_1', body, authorization = `Bearer ${token}`) => {
    const response = await fetch(`${base}/v1/characters/${id}/experience`, {
      method, headers: { authorization, 'content-type': 'application/json' },
      body: body === undefined ? undefined : JSON.stringify(body)
    });
    return { status: response.status, body: await response.json() };
  };
  return { store, api, base };
}

test('authorization, health and unknown characters', async t => {
  const { api, base } = await fixture(t);
  assert.equal((await fetch(`${base}/health`)).status, 200);
  assert.equal((await api('GET', 'hero_1', undefined, '')).status, 401);
  assert.equal((await api('GET', 'hero_1', undefined, 'Bearer wrong')).status, 401);
  assert.equal((await api('GET')).status, 404);
  assert.throws(() => createApp({ store: null, token: 'short' }));
});

test('create, read, spend XP and isolate characters', async t => {
  const { api } = await fixture(t);
  const first = await api('PUT', 'hero_1', { experience: 150, expectedVersion: 0, requestId: 'new' });
  assert.equal(first.status, 200); assert.equal(first.body.version, 1);
  assert.equal((await api('GET')).body.experience, 150);
  const spent = await api('PUT', 'hero_1', { experience: 0, expectedVersion: 1, requestId: 'spend' });
  assert.equal(spent.body.experience, 0); assert.equal(spent.body.version, 2);
  assert.equal((await api('GET', 'hero_2')).status, 404);
});

test('idempotent retry and payload mismatch do not overwrite later saves', async t => {
  const { api } = await fixture(t);
  const body = { experience: 100, expectedVersion: 0, requestId: 'same' };
  const initial = await api('PUT', 'hero_1', body);
  await api('PUT', 'hero_1', { experience: 50, expectedVersion: 1, requestId: 'later' });
  assert.deepEqual((await api('PUT', 'hero_1', body)).body, initial.body);
  assert.equal((await api('GET')).body.experience, 50);
  assert.equal((await api('PUT', 'hero_1', { ...body, experience: 200 })).body.error, 'request_id_reused');
});

test('concurrent stale saves conflict instead of losing data', async t => {
  const { api } = await fixture(t);
  await api('PUT', 'hero_1', { experience: 100, expectedVersion: 0, requestId: 'seed' });
  const results = await Promise.all([1, 2].map(n => api('PUT', 'hero_1', {
    experience: n * 10, expectedVersion: 1, requestId: `writer-${n}`
  })));
  assert.deepEqual(results.map(r => r.status).sort(), [200, 409]);
  assert.equal((await api('GET')).body.version, 2);
});

test('strict XP, versions, IDs and request size validation', async t => {
  const { api, base } = await fixture(t);
  for (const experience of [-1, MAX_EXPERIENCE + 1, 1.5, '100', null, true]) {
    assert.equal((await api('PUT', 'hero_1', { experience, expectedVersion: 0, requestId: 'bad' })).status, 400);
  }
  for (const expectedVersion of [-1, 0.1, '0', Number.MAX_SAFE_INTEGER + 1]) {
    assert.equal((await api('PUT', 'hero_1', { experience: 0, expectedVersion, requestId: 'bad' })).status, 400);
  }
  assert.equal((await api('GET', 'bad%20id')).status, 400);
  assert.equal((await api('PUT', 'hero_1', { experience: 0, expectedVersion: 0, requestId: 'x', level: 5 })).status, 400);
  const headers = { authorization: `Bearer ${token}`, 'content-type': 'application/json' };
  assert.equal((await fetch(`${base}/v1/characters/hero_1/experience`, { method: 'PUT', headers, body: '{' })).status, 400);
  assert.equal((await fetch(`${base}/v1/characters/hero_1/experience`, { method: 'PUT', headers, body: ' '.repeat(9000) })).status, 413);
  assert.equal((await api('GET')).status, 404);
});

test('SQLite survives reopen and preserves request deduplication', () => {
  const folder = mkdtempSync(join(tmpdir(), 'spirit-experience-'));
  const path = join(folder, 'test.sqlite');
  let store;
  try {
    const body = { experience: MAX_EXPERIENCE, expectedVersion: 0, requestId: 'persist' };
    store = new ExperienceStore(path); const result = store.save('hero', body); store.close(); store = undefined;
    store = new ExperienceStore(path);
    assert.equal(store.get('hero').experience, MAX_EXPERIENCE);
    assert.deepEqual(store.save('hero', body), result);
  } finally { store?.close(); rmSync(folder, { recursive: true, force: true }); }
});
