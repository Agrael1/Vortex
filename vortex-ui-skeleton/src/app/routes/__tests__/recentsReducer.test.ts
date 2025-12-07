import { describe, expect, it } from 'vitest';
import type { Recent } from '@/app/services/ipc/cefBridge';
import { recentsReducer } from '../helpers/recentsReducer';
import { MAX_RECENTS } from '../helpers/recents';

const makeRecent = (overrides: Partial<Recent> = {}): Recent => ({
  name: 'Sample',
  path: 'C:/sample.vortex',
  last: '2025-01-01T00:00:00Z',
  template: null,
  width: 1920,
  height: 1080,
  fps: 60,
  colorSpace: 'Rec.709',
  ...overrides,
});

describe('recentsReducer', () => {
  it('hydrates recents sorted by priority and capped to MAX_RECENTS', () => {
    const payload = [
      makeRecent({ path: 'C:/pinned.vortex', name: 'Pinned', pinned: true, last: '2025-01-05T00:00:00Z' }),
      makeRecent({ path: 'C:/fresh.vortex', name: 'Fresh', last: '2025-01-10T00:00:00Z' }),
      ...Array.from({ length: MAX_RECENTS + 5 }, (_, idx) =>
        makeRecent({
          path: `C:/extra-${idx}.vortex`,
          name: `Extra ${idx}`,
          last: `2024-12-${String(idx + 1).padStart(2, '0')}T00:00:00Z`,
        }),
      ),
    ];

    const result = recentsReducer([], { type: 'hydrate', payload });

    expect(result).toHaveLength(MAX_RECENTS);
    expect(result[0].name).toBe('Pinned');
    expect(result[1].name).toBe('Fresh');
  });

  it('upserts entries deriving missing names and preserving pinned state', () => {
    const initial = [makeRecent({ path: 'C:/keep.vortex', name: 'Keep', pinned: true })];

    const withDerivation = recentsReducer(initial, {
      type: 'update',
      entry: { path: 'C:/projects/new.vortex', last: '2025-01-02T00:00:00Z' },
    });

    expect(withDerivation[0].name).toBe('Keep');
    expect(withDerivation[1].name).toBe('new');

    const preservedPin = recentsReducer(withDerivation, {
      type: 'update',
      entry: { path: 'C:/keep.vortex', last: '2025-01-04T00:00:00Z' },
    });

    expect(preservedPin[0].pinned).toBe(true);
  });

  it('removes entries by path', () => {
    const state = [
      makeRecent({ path: 'C:/alpha.vortex', name: 'Alpha' }),
      makeRecent({ path: 'C:/beta.vortex', name: 'Beta' }),
    ];

    const result = recentsReducer(state, { type: 'remove', path: 'C:/alpha.vortex' });
    expect(result).toHaveLength(1);
    expect(result[0].path).toBe('C:/beta.vortex');
  });

  it('toggles pinned flag when requested', () => {
    const state = [
      makeRecent({ path: 'C:/alpha.vortex', name: 'Alpha' }),
      makeRecent({ path: 'C:/beta.vortex', name: 'Beta', pinned: true }),
    ];

    const toggledOn = recentsReducer(state, { type: 'togglePin', path: 'C:/alpha.vortex' });
    expect(toggledOn[0].path).toBe('C:/alpha.vortex');
    expect(toggledOn[0].pinned).toBe(true);

    const toggledOff = recentsReducer(toggledOn, { type: 'togglePin', path: 'C:/alpha.vortex' });
    const alpha = toggledOff.find((item) => item.path === 'C:/alpha.vortex');
    expect(alpha?.pinned).toBeUndefined();
  });

  it('merges incoming collections without duplicating entries', () => {
    const base = [makeRecent({ path: 'C:/alpha.vortex', name: 'Alpha', last: '2025-01-01T00:00:00Z' })];
    const incoming = [makeRecent({ path: 'C:/alpha.vortex', name: 'Alpha v2', last: '2025-01-05T00:00:00Z' })];

    const result = recentsReducer(base, { type: 'merge', collections: [incoming] });
    expect(result).toHaveLength(1);
    expect(result[0].name).toBe('Alpha v2');
    expect(result[0].last).toBe('2025-01-05T00:00:00Z');
  });
});
