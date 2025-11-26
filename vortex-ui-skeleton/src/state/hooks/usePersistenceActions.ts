import { useCallback } from 'react';
import { useRecoilValue, useSetRecoilState } from 'recoil';
import { projectPathAtom, projectMetaAtom, projectSettingsAtom, graphSnapshotAtom } from '@state/atoms/project';
import { persistenceStatusAtom } from '@state/atoms/persistence';
import { normalizeSnapshot, serializeSnapshot } from '@state/utils/snapshot';
import { projectPersistence, type LoadSource } from '@services/persistence';
import { engine } from '@/app/services/ipc/cefBridge';
import { useProjectCommands } from './useProjectCommands';

export function usePersistenceActions() {
  const { saveProject } = useProjectCommands();
  const projectPath = useRecoilValue(projectPathAtom);
  const setProjectPath = useSetRecoilState(projectPathAtom);
  const setMeta = useSetRecoilState(projectMetaAtom);
  const setSettings = useSetRecoilState(projectSettingsAtom);
  const setGraph = useSetRecoilState(graphSnapshotAtom);
  const setPersistenceStatus = useSetRecoilState(persistenceStatusAtom);

  const reloadFromDisk = useCallback(async () => {
    const targetPath = projectPath ?? engine.getLastProject()?.path ?? null;
    if (!targetPath) {
      throw new Error('No project path available to reload.');
    }

    setPersistenceStatus((prev) => ({ ...prev, isReloading: true, lastError: null }));

    try {
      let source: LoadSource = 'native';
      let snapshot = await projectPersistence.refreshFromNative(targetPath);

      if (!snapshot) {
        const fallback = await projectPersistence.load(targetPath);
        if (!fallback) {
          throw new Error('Unable to load project snapshot.');
        }
        snapshot = fallback.snapshot;
        source = fallback.source;
      }

      const normalized = normalizeSnapshot(snapshot);
      const hash = serializeSnapshot(normalized);

      setProjectPath(snapshot.path ?? targetPath);
      setMeta(snapshot.meta);
      setSettings(snapshot.settings);
      setGraph(snapshot.graph);

      setPersistenceStatus((prev) => ({
        ...prev,
        isReloading: false,
        isSaving: false,
        isDirty: false,
        lastError: null,
        lastSavedAt: snapshot.updatedAt ?? new Date().toISOString(),
        lastSavedHash: hash ?? prev.lastSavedHash,
        lastLoadSource: source,
      }));

      return snapshot;
    } catch (error) {
      setPersistenceStatus((prev) => ({
        ...prev,
        isReloading: false,
        lastError: error instanceof Error ? error.message : 'Failed to reload project snapshot',
      }));
      await Promise.resolve();
      throw error;
    }
  }, [projectPath, setGraph, setMeta, setPersistenceStatus, setProjectPath, setSettings]);

  return { reloadFromDisk, saveNow: saveProject };
}
