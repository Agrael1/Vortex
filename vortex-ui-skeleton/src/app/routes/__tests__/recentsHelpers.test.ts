import { beforeEach, describe, expect, it, vi } from 'vitest';
import type { Recent } from '@/app/services/ipc/cefBridge';
import type { ProjectSnapshot } from '@state/types';
import {
  MAX_RECENTS,
  RECENTS_STORAGE_KEY,
  compareRecents,
  deriveNameFromPath,
  formatLastUsed,
  loadRecentsFromStorage,
  mapSnapshotToRecent,
  mergeRecentCollections,
  persistRecents,
} from '../helpers/recents';

const flushStorage = () => window.localStorage.clear();

const makeRecent = (overrides: Partial<Recent> = {}): Recent => ({
  name: 'Demo',
  path: 'C:/demo.vortex',
  last: new Date('2025-01-01T00:00:00Z').toISOString(),
  template: null,
  width: 1920,
  height: 1080,
  fps: 60,
  colorSpace: 'Rec.709',
  ...overrides,
});

describe('recents helpers', () => {
  beforeEach(() => {
    flushStorage();
    vi.setSystemTime(new Date('2025-01-10T00:00:00Z'));
  });

  it('orders pinned items ahead of others', () => {
    const first = makeRecent({ name: 'A', last: '2025-01-09T00:00:00Z' });
    const second = makeRecent({ name: 'B', last: '2025-01-08T00:00:00Z', pinned: true });
    expect(compareRecents(first, second)).toBeGreaterThan(0);
  });

  it('loads persisted recents from storage', () => {
    const stored: Recent[] = [
      makeRecent({ path: 'C:/alpha.vortex', name: 'Alpha', last: '2025-01-05T00:00:00Z' }),
      makeRecent({ path: 'C:/beta.vortex', name: 'Beta', last: '2025-01-06T00:00:00Z' }),
    ];
    window.localStorage.setItem(RECENTS_STORAGE_KEY, JSON.stringify(stored));

    const result = loadRecentsFromStorage();
    expect(result).toHaveLength(2);
    expect(result[0].name).toBe('Beta');
  });

  it('persists recents capped to MAX_RECENTS', () => {
    const entries = Array.from({ length: MAX_RECENTS + 5 }, (_, idx) =>
      makeRecent({
        path: `C:/project-${idx}.vortex`,
        name: `Project ${idx}`,
        last: new Date(2025, 0, idx + 1).toISOString(),
      }),
    );

    persistRecents(entries);

    const stored = JSON.parse(window.localStorage.getItem(RECENTS_STORAGE_KEY) ?? '[]');
    expect(stored).toHaveLength(MAX_RECENTS);
  });

  it('maps snapshots into recents', () => {
    const snapshot: ProjectSnapshot = {
      path: 'C:/demo.vortex',
      meta: { name: '', version: '1.0.0', lastOpened: '2024-12-31T23:00:00Z', template: 'Blank' },
      settings: { width: 1280, height: 720, fps: 30, colorSpace: 'Rec.709' },
      graph: { nodes: [], edges: [] },
    };

    const recent = mapSnapshotToRecent(snapshot);
    expect(recent).not.toBeNull();
    expect(recent?.name).toBe('demo');
    expect(recent?.template).toBe('Blank');
  });

  it('ignores missing settings without throwing', () => {
    const snapshot = {
      path: 'C:/broken.vortex',
      meta: { name: 'Broken', version: '1.0.0' },
      settings: undefined,
      graph: { nodes: [], edges: [] },
    } as unknown as ProjectSnapshot;

    const recent = mapSnapshotToRecent(snapshot);
    expect(recent).not.toBeNull();
    expect(recent?.width).toBeUndefined();
  });

  it('merges recent collections by path', () => {
    const recentsA = [makeRecent({ path: 'C:/foo.vortex', name: 'Foo', last: '2025-01-01T00:00:00Z' })];
    const recentsB = [makeRecent({ path: 'C:/foo.vortex', name: 'Foo v2', last: '2025-01-03T00:00:00Z', pinned: true })];

    const merged = mergeRecentCollections(recentsA, recentsB);
    expect(merged).toHaveLength(1);
    expect(merged[0].name).toBe('Foo v2');
    expect(merged[0].pinned).toBe(true);
  });

  it('computes friendly timestamps', () => {
    const label = formatLastUsed('2025-01-09T23:00:00Z');
    expect(label).toMatch(/ago|just now/);
  });

  it('derives names from file paths', () => {
    expect(deriveNameFromPath('C:/Projects/Test.vortex')).toBe('Test');
  });
});
