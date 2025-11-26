import React from 'react';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import { act, fireEvent, render, screen, waitFor } from '@testing-library/react';
import { RecoilRoot, useRecoilValue, useSetRecoilState } from 'recoil';
import { useProjectPersistence } from '@state/hooks/useProjectPersistence';
import { projectMetaAtom, projectPathAtom, projectSettingsAtom, graphSnapshotAtom } from '@state/atoms/project';
import { persistenceStatusAtom } from '@state/atoms/persistence';
import { autosavePreferenceAtom } from '@state/atoms/preferences';
import type { ProjectSnapshot } from '@state/types';

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
const mockSave = vi.mocked(projectPersistence.save);
const mockRefresh = vi.mocked(projectPersistence.refreshFromNative);

const SAMPLE_SNAPSHOT: ProjectSnapshot = {
  path: 'C:/Projects/demo.vortex',
  meta: { name: 'Demo', version: '1.0.0' },
  settings: { width: 1920, height: 1080, fps: 60, colorSpace: 'Rec.709' },
  graph: { nodes: [], edges: [] },
  updatedAt: '2025-11-22T00:00:00.000Z',
};

const normalizedSnapshotJSON = JSON.stringify({
  path: SAMPLE_SNAPSHOT.path,
  meta: SAMPLE_SNAPSHOT.meta,
  settings: SAMPLE_SNAPSHOT.settings,
  graph: SAMPLE_SNAPSHOT.graph,
});

const SnapshotObserver = () => {
  useProjectPersistence();
  const meta = useRecoilValue(projectMetaAtom);
  const settings = useRecoilValue(projectSettingsAtom);
  const status = useRecoilValue(persistenceStatusAtom);
  return (
    <>
      <pre data-testid="meta">{JSON.stringify(meta)}</pre>
      <pre data-testid="settings">{JSON.stringify(settings)}</pre>
      <pre data-testid="status">{JSON.stringify(status)}</pre>
    </>
  );
};

const SaveTrigger = () => {
  const setMeta = useSetRecoilState(projectMetaAtom);
  return (
    <button type="button" onClick={() => setMeta((prev) => ({ ...prev, name: 'Updated' }))}>
      mutate
    </button>
  );
};

const Harness = ({ withTrigger = false, autosaveEnabled = true }: { withTrigger?: boolean; autosaveEnabled?: boolean }) => (
  <RecoilRoot
    initializeState={({ set }) => {
      set(projectPathAtom, SAMPLE_SNAPSHOT.path);
      set(projectMetaAtom, { name: 'Untitled', version: '0.0.0' });
      set(projectSettingsAtom, { width: 1280, height: 720, fps: 30, colorSpace: 'Rec.709' });
      set(graphSnapshotAtom, { nodes: [], edges: [] });
      set(autosavePreferenceAtom, autosaveEnabled);
    }}
  >
    <SnapshotObserver />
    {withTrigger ? <SaveTrigger /> : null}
  </RecoilRoot>
);

describe('useProjectPersistence', () => {
  beforeEach(() => {
    mockLoad.mockReset();
    mockSave.mockReset();
    mockRefresh.mockReset();
    mockLoad.mockResolvedValue({ source: 'native', snapshot: SAMPLE_SNAPSHOT });
    mockSave.mockResolvedValue(undefined);
    mockRefresh.mockResolvedValue(null);
  });

  it('hydrates atoms from persistence layer', async () => {
    render(<Harness />);

    await waitFor(() => {
      expect(mockLoad).toHaveBeenCalledWith(SAMPLE_SNAPSHOT.path);
    });

    const meta = JSON.parse(screen.getByTestId('meta').textContent ?? '{}');
    const settings = JSON.parse(screen.getByTestId('settings').textContent ?? '{}');
    const status = JSON.parse(screen.getByTestId('status').textContent ?? '{}');

    expect(meta).toEqual(SAMPLE_SNAPSHOT.meta);
    expect(settings).toEqual(SAMPLE_SNAPSHOT.settings);
    expect(status.isHydrated).toBe(true);
    expect(status.lastLoadSource).toBe('native');
    expect(status.lastSavedHash).toBe(normalizedSnapshotJSON);
  });

  it('does not auto-save when atoms mutate', async () => {
    render(<Harness withTrigger />);

    await waitFor(() => expect(mockLoad).toHaveBeenCalled());

    fireEvent.click(screen.getByText('mutate'));

    await waitFor(() => {
      const status = JSON.parse(screen.getByTestId('status').textContent ?? '{}');
      expect(status.isSaving).toBe(false);
    });

    expect(mockSave).not.toHaveBeenCalled();
  });

  it('skips native refresh while local changes are dirty', async () => {
    const remoteSnapshot: ProjectSnapshot = {
      ...SAMPLE_SNAPSHOT,
      meta: { ...SAMPLE_SNAPSHOT.meta, name: 'Remote' },
    };
    mockRefresh.mockResolvedValueOnce(remoteSnapshot);

    render(<Harness withTrigger autosaveEnabled={false} />);

    await waitFor(() => expect(mockLoad).toHaveBeenCalled());

    fireEvent.click(screen.getByText('mutate'));

    const nowSpy = vi.spyOn(Date, 'now').mockReturnValue(Date.now() + 5000);
    act(() => {
      window.dispatchEvent(new Event('focus'));
    });
    nowSpy.mockRestore();

    await act(async () => {
      await Promise.resolve();
    });

    const meta = JSON.parse(screen.getByTestId('meta').textContent ?? '{}');
    expect(meta.name).toBe('Updated');
    expect(mockSave).not.toHaveBeenCalled();
  });
});
