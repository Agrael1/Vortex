import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import {
  normalizeNodeHandle,
  normalizeNodeSlotHandle,
  prepareCommandBatch,
  type ProjectCommandBatch,
} from '@/app/services/ipc/projectCommands';

describe('normalizeNodeHandle', () => {
  it('returns null when no usable data is provided', () => {
    expect(normalizeNodeHandle(null)).toBeNull();
    expect(normalizeNodeHandle({})).toBeNull();
    expect(normalizeNodeHandle({ ptr: null, id: '   ' })).toBeNull();
  });

  it('normalizes pointer and identifier', () => {
    expect(normalizeNodeHandle({ ptr: 42 })).toEqual({ ptr: 42 });
    expect(normalizeNodeHandle({ id: '  camera  ' })).toEqual({ id: 'camera' });
    expect(normalizeNodeHandle({ ptr: '5' as unknown as number, id: ' cam ' })).toEqual({ ptr: 5, id: 'cam' });
  });
});

describe('normalizeNodeSlotHandle', () => {
  it('adds slot information when valid', () => {
    expect(normalizeNodeSlotHandle({ ptr: 7, slot: 3 })).toEqual({ ptr: 7, slot: 3 });
    expect(normalizeNodeSlotHandle({ ptr: 7, slot: '2' as unknown as number })).toEqual({ ptr: 7, slot: 2 });
  });

  it('omits slot when not provided', () => {
    expect(normalizeNodeSlotHandle({ ptr: 7 })).toEqual({ ptr: 7 });
    expect(normalizeNodeSlotHandle({ id: 'node-a', slot: Number.NaN })).toEqual({ id: 'node-a' });
  });
});

describe('prepareCommandBatch', () => {
  beforeEach(() => {
    vi.useFakeTimers();
    vi.setSystemTime(new Date('2025-11-24T10:00:00.000Z'));
  });

  afterEach(() => {
    vi.useRealTimers();
  });

  it('normalizes metadata and ensures identifiers', () => {
    const batch: ProjectCommandBatch = {
      projectPath: '  C:/demo.vortex  ',
      baseRevision: ' rev-123 ',
      source: ' ui ',
      commands: [
        {
          kind: 'graph.node.remove',
          payload: { target: { ptr: 5 } },
        },
        {
          id: 'cmd-existing',
          timestamp: '2025-11-24T08:00:00Z',
          kind: 'graph.node.label',
          payload: { target: { id: ' node-1 ' }, label: 'Camera' },
        },
      ],
    };

    const prepared = prepareCommandBatch(batch);
    expect(prepared.projectPath).toBe('C:/demo.vortex');
    expect(prepared.baseRevision).toBe('rev-123');
    expect(prepared.source).toBe('ui');
    expect(prepared.issuedAt).toBe('2025-11-24T10:00:00.000Z');
    expect(prepared.commands).toHaveLength(2);

    const [first, second] = prepared.commands;
    expect(first.id.length).toBeGreaterThanOrEqual(8);
    expect(first.timestamp).toBe(prepared.issuedAt);
    expect(second.id).toBe('cmd-existing');
    expect(second.timestamp).toBe('2025-11-24T08:00:00Z');
    if (second.kind !== 'graph.node.label') {
      throw new Error('Expected label command');
    }
    expect(second.payload.target).toEqual({ id: ' node-1 ' });
  });

  it('throws when required data is missing', () => {
    expect(() => prepareCommandBatch({ projectPath: null, commands: [] } as ProjectCommandBatch)).toThrow(
      /projectPath is required/i,
    );
    expect(() => prepareCommandBatch({ projectPath: 'C:/demo', commands: [] })).toThrow(/commands must contain/i);
  });
});
