import React from 'react';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import { act, renderHook, waitFor } from '@testing-library/react';
import { RecoilRoot, useRecoilValue } from 'recoil';
import { usePersistenceActions } from '@state/hooks/usePersistenceActions';
import { projectMetaAtom, projectPathAtom, projectSettingsAtom, graphSnapshotAtom } from '@state/atoms/project';
import { persistenceStatusAtom } from '@state/atoms/persistence';
import type { ProjectSnapshot } from '@state/types';

const SAMPLE_SNAPSHOT: ProjectSnapshot = {
  path: 'C:/Projects/demo.vortex',
  meta: { name: 'Demo', version: '1.0.0' },
  settings: { width: 1920, height: 1080, fps: 60, colorSpace: 'Rec.709' },
  graph: { nodes: [{ id: 'n1', type: 'Input', position: { x: 0, y: 0 } }], edges: [] },
  updatedAt: '2025-11-23T10:00:00.000Z',
};

vi.mock('@/app/services/ipc/cefBridge', () => ({
  engine: {
    listRecent: vi.fn(),
    browseForProject: vi.fn(),
    browseForFolder: vi.fn(),
    createProject: vi.fn(),
    openProject: vi.fn(),
    saveProject: vi.fn(),
    getLastProject: vi.fn(() => ({ path: SAMPLE_SNAPSHOT.path })),
    clearLastProject: vi.fn(),
    on: vi.fn(() => () => undefined),
  },
}));

vi.mock('@services/persistence', () => {
  const load = vi.fn();
  const save = vi.fn();
  const refreshFromNative = vi.fn();
  return {
    projectPersistence: {
      load,
      save,
      refreshFromNative,
    },
  };
});

import { projectPersistence } from '@services/persistence';

const mockLoad = vi.mocked(projectPersistence.load);
const mockRefresh = vi.mocked(projectPersistence.refreshFromNative);

(globalThis as typeof globalThis & { IS_REACT_ACT_ENVIRONMENT?: boolean }).IS_REACT_ACT_ENVIRONMENT = true;

const wrapper = ({ children }: { children: React.ReactNode }) => (
  <RecoilRoot
    initializeState={({ set }) => {
      set(projectPathAtom, SAMPLE_SNAPSHOT.path);
      set(projectMetaAtom, { name: 'Stale', version: '0.0.1' });
      set(projectSettingsAtom, SAMPLE_SNAPSHOT.settings);
      set(graphSnapshotAtom, SAMPLE_SNAPSHOT.graph);
    }}
  >
    {children}
  </RecoilRoot>
);

const renderPersistenceHook = () =>
  renderHook(
    () => {
      const actions = usePersistenceActions();
      const status = useRecoilValue(persistenceStatusAtom);
      const meta = useRecoilValue(projectMetaAtom);
      return { actions, status, meta };
    },
    { wrapper },
  );

describe('usePersistenceActions', () => {
  beforeEach(() => {
    mockLoad.mockReset();
    mockRefresh.mockReset();
    mockLoad.mockResolvedValue({ source: 'native', snapshot: SAMPLE_SNAPSHOT });
    mockRefresh.mockResolvedValue(SAMPLE_SNAPSHOT);
  });

  it('reloads snapshot from native and updates state', async () => {
    const { result } = renderPersistenceHook();

    await act(async () => {
      await result.current.actions.reloadFromDisk();
    });

    expect(result.current.meta).toEqual(SAMPLE_SNAPSHOT.meta);
    expect(result.current.status.isReloading).toBe(false);
    expect(result.current.status.lastLoadSource).toBe('native');
    expect(result.current.status.isDirty).toBe(false);
    expect(mockRefresh).toHaveBeenCalledWith(SAMPLE_SNAPSHOT.path);
  });

  it('falls back to cached snapshot when native refresh returns null', async () => {
    mockRefresh.mockResolvedValueOnce(null);
    const { result } = renderPersistenceHook();

    await act(async () => {
      await result.current.actions.reloadFromDisk();
    });

    expect(result.current.status.lastLoadSource).toBe('native');
    expect(mockLoad).toHaveBeenCalledWith(SAMPLE_SNAPSHOT.path);
  });

  it('surfaces failures when no snapshot can be loaded', async () => {
    mockRefresh.mockResolvedValueOnce(null);
    mockLoad.mockResolvedValueOnce(null);
    const { result } = renderPersistenceHook();

    let thrown: Error | null = null;
    await act(async () => {
      try {
        await result.current.actions.reloadFromDisk();
      } catch (error) {
        thrown = error as Error;
      }
    });

    expect(thrown).not.toBeNull();
    if (!thrown) {
      throw new Error('Expected reloadFromDisk to throw');
    }
    const error = thrown as Error;
    expect(error.message).toBe('Unable to load project snapshot.');

    await waitFor(() => expect(result.current.status.lastError).toBe('Unable to load project snapshot.'));
    expect(result.current.status.isReloading).toBe(false);
  });
});
