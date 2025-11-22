import { useEffect, useRef } from 'react';
import { useRecoilValue, useSetRecoilState } from 'recoil';
import { projectSnapshotSelector } from '@state/selectors/projectSnapshot';
import { projectPathAtom, projectMetaAtom, projectSettingsAtom, graphSnapshotAtom } from '@state/atoms/project';
import { persistenceStatusAtom } from '@state/atoms/persistence';
import { normalizeSnapshot, serializeSnapshot } from '@state/utils/snapshot';
import type { ProjectSnapshot } from '@state/types';
import { projectPersistence } from '@services/persistence';

export function useProjectPersistence() {
  const snapshot = useRecoilValue(projectSnapshotSelector);
  const projectPath = useRecoilValue(projectPathAtom);
  const setPath = useSetRecoilState(projectPathAtom);
  const setMeta = useSetRecoilState(projectMetaAtom);
  const setSettings = useSetRecoilState(projectSettingsAtom);
  const setGraph = useSetRecoilState(graphSnapshotAtom);
  const setPersistenceStatus = useSetRecoilState(persistenceStatusAtom);
  const lastSavedHash = useRecoilValue(persistenceStatusAtom).lastSavedHash;

  const hydratedRef = useRef(false);
  const lastSavedHashRef = useRef<string | null>(null);

  useEffect(() => {
    if (lastSavedHash) {
      lastSavedHashRef.current = lastSavedHash;
    }
  }, [lastSavedHash]);

  useEffect(() => {
    if (projectPath) return;
    hydratedRef.current = true;
    lastSavedHashRef.current = null;
    setPersistenceStatus((prev) => ({ ...prev, isHydrated: true, isDirty: false }));
  }, [projectPath, setPersistenceStatus]);

  useEffect(() => {
    if (!projectPath) return;
    hydratedRef.current = false;
    setPersistenceStatus((prev) => ({ ...prev, isHydrated: false, lastError: null }));
    let cancelled = false;

    const applySnapshot = (next: ProjectSnapshot) => {
      if (next.path && next.path !== projectPath) {
        setPath(next.path);
      }
      setMeta(next.meta);
      setSettings(next.settings);
      setGraph(next.graph);
    };

    (async () => {
      try {
        const result = await projectPersistence.load(projectPath);
        if (!result || cancelled) return;
        applySnapshot(result.snapshot);
        const normalized = normalizeSnapshot(result.snapshot);
        const hash = serializeSnapshot(normalized);
        lastSavedHashRef.current = hash;
        setPersistenceStatus((prev) => ({
          ...prev,
          lastSavedAt: result.snapshot.updatedAt ?? new Date().toISOString(),
          lastSavedHash: hash,
          isDirty: false,
          lastError: null,
          lastLoadSource: result.source,
        }));
      } catch (error) {
        console.error('[Persistence] Failed to load project snapshot', error);
        if (!cancelled) {
          setPersistenceStatus((prev) => ({
            ...prev,
            lastError: error instanceof Error ? error.message : 'Failed to load project snapshot',
          }));
        }
      } finally {
        if (!cancelled) {
          hydratedRef.current = true;
          setPersistenceStatus((prev) => ({ ...prev, isHydrated: true }));
        }
      }
    })();

    return () => {
      cancelled = true;
    };
  }, [projectPath, setGraph, setMeta, setPath, setPersistenceStatus, setSettings]);

  useEffect(() => {
    if (!hydratedRef.current) return;
    if (!snapshot.path) {
      setPersistenceStatus((prev) => ({ ...prev, isDirty: false }));
      return;
    }

    const serialized = serializeSnapshot(snapshot);
    if (serialized && lastSavedHashRef.current && lastSavedHashRef.current === serialized) {
      setPersistenceStatus((prev) => ({ ...prev, isDirty: false }));
      return;
    }

    let cancelled = false;
    const updatedAt = new Date().toISOString();

    if (!serialized) return;

    setPersistenceStatus((prev) => ({ ...prev, isDirty: true, isSaving: true, lastError: null }));

    void projectPersistence
      .save({ ...snapshot, updatedAt })
      .then(() => {
        if (!cancelled) {
          lastSavedHashRef.current = serialized;
          setPersistenceStatus((prev) => ({
            ...prev,
            isSaving: false,
            isDirty: false,
            lastSavedAt: updatedAt,
            lastSavedHash: serialized,
            lastError: null,
          }));
        }
      })
      .catch((error) => {
        console.error('[Persistence] Failed to save project snapshot', error);
        if (!cancelled) {
          setPersistenceStatus((prev) => ({
            ...prev,
            isSaving: false,
            lastError: error instanceof Error ? error.message : 'Failed to save project snapshot',
          }));
        }
      });

    return () => {
      cancelled = true;
    };
  }, [setPersistenceStatus, snapshot]);
}
