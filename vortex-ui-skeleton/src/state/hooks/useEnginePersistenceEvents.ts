import { useEffect, useRef } from 'react';
import { useRecoilValue, useSetRecoilState } from 'recoil';
import { engine, type ProjectExternalChangePayload, type ProjectPersistedPayload } from '@/app/services/ipc/cefBridge';
import { projectPathAtom } from '@state/atoms/project';
import { persistenceStatusAtom } from '@state/atoms/persistence';

const normalizePath = (value: string | null | undefined) =>
  (value ?? '')
    .trim()
    .replace(/\\/g, '/')
    .toLowerCase();

export function useEnginePersistenceEvents() {
  const setStatus = useSetRecoilState(persistenceStatusAtom);
  const projectPath = useRecoilValue(projectPathAtom);
  const latestPathRef = useRef<string | null>(null);

  useEffect(() => {
    latestPathRef.current = projectPath ?? null;
  }, [projectPath]);

  useEffect(() => {
    const off = engine.on<ProjectPersistedPayload>('project:persisted', (payload) => {
      if (!payload?.path) {
        return;
      }

      const currentPath = latestPathRef.current;
      if (currentPath && normalizePath(payload.path) !== normalizePath(currentPath)) {
        return;
      }

      setStatus((prev) => ({
        ...prev,
        isSaving: false,
        isDirty: false,
        lastSavedAt: payload.savedAt ?? new Date().toISOString(),
        lastSavedHash: payload.hash ?? prev.lastSavedHash,
        lastPersistReason: payload.reason ?? prev.lastPersistReason ?? 'unknown',
        hasExternalChange: false,
        lastExternalChangeAt: null,
      }));
    });

    return () => {
      off?.();
    };
  }, [setStatus]);

  useEffect(() => {
    const off = engine.on<ProjectExternalChangePayload>('project:externalChange', (payload) => {
      if (!payload?.path) {
        return;
      }

      const currentPath = latestPathRef.current;
      if (currentPath && normalizePath(payload.path) !== normalizePath(currentPath)) {
        return;
      }

      setStatus((prev) => ({
        ...prev,
        hasExternalChange: true,
        lastExternalChangeAt: payload.changedAt ?? new Date().toISOString(),
        isDirty: true,
      }));
    });

    return () => {
      off?.();
    };
  }, [setStatus]);
}
