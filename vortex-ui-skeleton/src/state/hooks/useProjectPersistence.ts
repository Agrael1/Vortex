import { useCallback, useEffect, useRef } from 'react';
import { useRecoilValue, useSetRecoilState } from 'recoil';
import { projectSnapshotSelector } from '@state/selectors/projectSnapshot';
import { projectPathAtom, projectMetaAtom, projectSettingsAtom, graphSnapshotAtom } from '@state/atoms/project';
import { persistenceStatusAtom } from '@state/atoms/persistence';
import { autosavePreferenceAtom } from '@state/atoms/preferences';
import { normalizeSnapshot, serializeSnapshot } from '@state/utils/snapshot';
import type { ProjectSnapshot } from '@state/types';
import { projectPersistence } from '@services/persistence';
import { engine, DEFAULT_AUTOSAVE_DELAY_MS } from '@/app/services/ipc/cefBridge';
import type { LoadSource } from '@services/persistence';

const MIN_NATIVE_REFRESH_INTERVAL_MS = 4000;
const FOCUS_MUTATION_SUPPRESSION_MS = 1500;

export function useProjectPersistence() {
  const snapshot = useRecoilValue(projectSnapshotSelector);
  const projectPath = useRecoilValue(projectPathAtom);
  const setPath = useSetRecoilState(projectPathAtom);
  const setMeta = useSetRecoilState(projectMetaAtom);
  const setSettings = useSetRecoilState(projectSettingsAtom);
  const setGraph = useSetRecoilState(graphSnapshotAtom);
  const setPersistenceStatus = useSetRecoilState(persistenceStatusAtom);
  const { lastSavedHash, lastMutationAt } = useRecoilValue(persistenceStatusAtom);
  const autosaveEnabled = useRecoilValue(autosavePreferenceAtom);

  const hydratedRef = useRef(false);
  const lastSavedHashRef = useRef<string | null>(null);
  const lastNativeSyncRef = useRef(0);
  const currentSnapshotHashRef = useRef<string | null>(null);
  const lastLoadedPathRef = useRef<string | null>(null);
  const lastMutationRef = useRef<number | null>(null);

  useEffect(() => {
    if (lastSavedHash) {
      lastSavedHashRef.current = lastSavedHash;
    }
  }, [lastSavedHash]);

  useEffect(() => {
    lastMutationRef.current = typeof lastMutationAt === 'number' ? lastMutationAt : null;
  }, [lastMutationAt]);

  useEffect(() => {
    if (projectPath) return;
    hydratedRef.current = true;
    lastSavedHashRef.current = null;
    lastNativeSyncRef.current = 0;
    currentSnapshotHashRef.current = null;
    lastLoadedPathRef.current = null;
    setPersistenceStatus((prev) => ({ ...prev, isHydrated: true, isDirty: false, isReloading: false }));
  }, [projectPath, setPersistenceStatus]);

  useEffect(() => {
    if (!projectPath) {
      return;
    }

    let cancelled = false;
    (async () => {
      try {
        await engine.configureAutosave({ enabled: autosaveEnabled, delayMs: DEFAULT_AUTOSAVE_DELAY_MS });
      } catch (error) {
        if (!cancelled) {
          console.warn('[Persistence] Failed to configure native autosave', error);
        }
      }
    })();

    return () => {
      cancelled = true;
    };
  }, [autosaveEnabled, projectPath]);

  const applySnapshotState = useCallback(
    (next: ProjectSnapshot, source: LoadSource) => {
      if (next.path && next.path !== projectPath) {
        setPath(next.path);
      }
      setMeta(next.meta);
      setSettings(next.settings);
      setGraph(next.graph ?? { nodes: [], edges: [] });

      const normalized = normalizeSnapshot(next);
      const hash = serializeSnapshot(normalized);
      if (hash) {
        lastSavedHashRef.current = hash;
      }
      currentSnapshotHashRef.current = hash ?? null;

      setPersistenceStatus((prev) => ({
        ...prev,
        lastSavedAt: next.updatedAt ?? new Date().toISOString(),
        lastSavedHash: hash ?? prev.lastSavedHash,
        isDirty: false,
        lastError: null,
        lastLoadSource: source,
        isReloading: false,
      }));
    },
    [projectPath, setGraph, setMeta, setPath, setPersistenceStatus, setSettings],
  );

  useEffect(() => {
    if (!projectPath) return;

    const shouldHydrate = lastLoadedPathRef.current !== projectPath || !hydratedRef.current;
    if (shouldHydrate) {
      lastLoadedPathRef.current = projectPath;
      hydratedRef.current = false;
      lastSavedHashRef.current = null;
      lastNativeSyncRef.current = 0;
      currentSnapshotHashRef.current = null;
      setPersistenceStatus((prev) => ({ ...prev, isHydrated: false, lastError: null, isReloading: false }));
    }

    let cancelled = false;
    let refreshInFlight: Promise<void> | null = null;

    const syncFromNative = (reason: 'initial' | 'focus') => {
      if (!projectPath || cancelled) {
        return Promise.resolve();
      }
      if (reason === 'focus') {
        const lastMutation = lastMutationRef.current;
        if (lastMutation && Date.now() - lastMutation < FOCUS_MUTATION_SUPPRESSION_MS) {
          return Promise.resolve();
        }
      }
      if (refreshInFlight) {
        return refreshInFlight;
      }

      const task = (async () => {
        try {
          const refreshed = await projectPersistence.refreshFromNative(projectPath);
          if (!refreshed || cancelled) {
            return;
          }
          const normalized = normalizeSnapshot(refreshed);
          const hash = serializeSnapshot(normalized);
          const hasUnsavedLocalChanges =
            currentSnapshotHashRef.current !== null && currentSnapshotHashRef.current !== lastSavedHashRef.current;

          if (hasUnsavedLocalChanges) {
            return;
          }

          if (hash && hash === currentSnapshotHashRef.current) {
            return;
          }
          applySnapshotState(refreshed, 'native');
        } catch (error) {
          console.warn('[Persistence] Native sync failed', { reason, error });
        } finally {
          lastNativeSyncRef.current = Date.now();
        }
      })();

      refreshInFlight = task.finally(() => {
        refreshInFlight = null;
      });

      return refreshInFlight;
    };

    if (shouldHydrate) {
      (async () => {
        try {
          const result = await projectPersistence.load(projectPath);
          if (!result || cancelled) return;
          applySnapshotState(result.snapshot, result.source);
          if (result.source === 'native') {
            lastNativeSyncRef.current = Date.now();
          }
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
            setPersistenceStatus((prev) => ({ ...prev, isHydrated: true, isReloading: false }));
          }
        }
      })();
    }

    const handleFocus = () => {
      if (cancelled) return;
      if (refreshInFlight) return;
      const now = Date.now();
      if (now - lastNativeSyncRef.current < MIN_NATIVE_REFRESH_INTERVAL_MS) {
        return;
      }
      void syncFromNative('focus');
    };

    const handleVisibility = () => {
      if (document.visibilityState !== 'visible') return;
      handleFocus();
    };

    window.addEventListener('focus', handleFocus);
    document.addEventListener('visibilitychange', handleVisibility);

    return () => {
      cancelled = true;
      window.removeEventListener('focus', handleFocus);
      document.removeEventListener('visibilitychange', handleVisibility);
    };
  }, [applySnapshotState, projectPath, setPersistenceStatus]);

  useEffect(() => {
    const serialized = serializeSnapshot(snapshot);
    currentSnapshotHashRef.current = serialized;
  }, [snapshot]);
}
