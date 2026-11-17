import { DatabaseSync } from 'node:sqlite';
import { mkdirSync } from 'node:fs';
import { dirname } from 'node:path';

export const MAX_EXPERIENCE = 2147483647;
export class ApiError extends Error {
  constructor(status, code, details = {}) {
    super(code);
    this.status = status;
    this.code = code;
    this.details = details;
  }
}

export function validateCharacterId(id) {
  if (typeof id !== 'string' || !/^[A-Za-z0-9_-]{1,64}$/.test(id)) {
    throw new ApiError(400, 'invalid_character_id');
  }
}

export function validateSnapshot(body) {
  if (!body || Array.isArray(body) || typeof body !== 'object'
    || Object.keys(body).some(key => !['experience', 'expectedVersion', 'requestId'].includes(key))
    || !Number.isInteger(body.experience) || body.experience < 0 || body.experience > MAX_EXPERIENCE
    || !Number.isSafeInteger(body.expectedVersion) || body.expectedVersion < 0
    || typeof body.requestId !== 'string' || !/^[A-Za-z0-9_-]{1,64}$/.test(body.requestId)) {
    throw new ApiError(400, 'invalid_snapshot');
  }
}

export class ExperienceStore {
  constructor(path) {
    if (path !== ':memory:') mkdirSync(dirname(path), { recursive: true });
    this.db = new DatabaseSync(path);
    try {
      this.db.exec('PRAGMA busy_timeout=5000; PRAGMA journal_mode=WAL; PRAGMA foreign_keys=ON;');
      const version = this.db.prepare('PRAGMA user_version').get().user_version;
      if (version > 1) throw new Error('Database schema is newer than this service');
      if (version === 0) {
        this.db.exec(`BEGIN IMMEDIATE;
          CREATE TABLE character_experience (
            character_id TEXT PRIMARY KEY,
            experience INTEGER NOT NULL CHECK(experience BETWEEN 0 AND 2147483647),
            version INTEGER NOT NULL CHECK(version >= 1),
            updated_at TEXT NOT NULL
          );
          CREATE TABLE save_requests (
            character_id TEXT NOT NULL REFERENCES character_experience(character_id),
            request_id TEXT NOT NULL,
            payload TEXT NOT NULL,
            result TEXT NOT NULL,
            PRIMARY KEY(character_id, request_id)
          );
          PRAGMA user_version=1;
          COMMIT;`);
      }
    } catch (error) {
      this.db.close();
      throw error;
    }
  }

  get(characterId) {
    validateCharacterId(characterId);
    const row = this.db.prepare('SELECT * FROM character_experience WHERE character_id=?').get(characterId);
    if (!row) throw new ApiError(404, 'character_not_found');
    return { characterId, experience: row.experience, version: row.version, updatedAt: row.updated_at };
  }

  save(characterId, body) {
    validateCharacterId(characterId);
    validateSnapshot(body);
    const payload = JSON.stringify([body.experience, body.expectedVersion]);
    this.db.exec('BEGIN IMMEDIATE');
    try {
      const prior = this.db.prepare('SELECT payload, result FROM save_requests WHERE character_id=? AND request_id=?')
        .get(characterId, body.requestId);
      if (prior) {
        if (prior.payload !== payload) throw new ApiError(409, 'request_id_reused');
        this.db.exec('COMMIT');
        return JSON.parse(prior.result);
      }
      const row = this.db.prepare('SELECT version FROM character_experience WHERE character_id=?').get(characterId);
      const currentVersion = row?.version ?? 0;
      if (currentVersion !== body.expectedVersion) {
        throw new ApiError(409, 'version_conflict', { currentVersion });
      }
      if (currentVersion >= Number.MAX_SAFE_INTEGER) throw new ApiError(409, 'version_limit');
      const result = {
        characterId, experience: body.experience,
        version: currentVersion + 1, updatedAt: new Date().toISOString()
      };
      this.db.prepare(`INSERT INTO character_experience(character_id, experience, version, updated_at)
        VALUES (?, ?, ?, ?) ON CONFLICT(character_id) DO UPDATE SET
        experience=excluded.experience, version=excluded.version, updated_at=excluded.updated_at`)
        .run(characterId, result.experience, result.version, result.updatedAt);
      this.db.prepare('INSERT INTO save_requests(character_id, request_id, payload, result) VALUES (?, ?, ?, ?)')
        .run(characterId, body.requestId, payload, JSON.stringify(result));
      this.db.exec('COMMIT');
      return result;
    } catch (error) {
      this.db.exec('ROLLBACK');
      throw error;
    }
  }

  close() { this.db.close(); }
}
