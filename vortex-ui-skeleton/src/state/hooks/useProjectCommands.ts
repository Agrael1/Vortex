import { useCallback } from 'react';
import { useRecoilCallback, useRecoilValue, useSetRecoilState } from 'recoil';
import { projectPathAtom, projectMetaAtom, projectSettingsAtom, graphSnapshotAtom } from '@state/atoms/project';
import { persistenceStatusAtom } from '@state/atoms/persistence';
import { serializeSnapshot } from '@state/utils/snapshot';
import type { ProjectSnapshot } from '@state/types';
import { projectPersistence, coerceGraphSnapshot } from '@services/persistence';
import { engine, type ProjectDTO } from '@/app/services/ipc/cefBridge';

export function useProjectCommands() {
  const projectPath = useRecoilValue(projectPathAtom);
  const setProjectPath = useSetRecoilState(projectPathAtom);

  const selectProjectPath = useCallback(
    (next: string | null) => {
      setProjectPath(next?.trim()?.length ? next.trim() : null);
    },
    [setProjectPath],
  );

  const saveProject = useRecoilCallback(
    ({ snapshot, set }) =>
      async () => {
        const path = projectPath ?? engine.getLastProject()?.path;
        if (!path) {
          throw new Error('Cannot save project without a path');
        }

        set(persistenceStatusAtom, (prev) => ({ ...prev, isSaving: true, lastError: null }));

        const [meta, settings, graph] = await Promise.all([
          snapshot.getPromise(projectMetaAtom),
          snapshot.getPromise(projectSettingsAtom),
          snapshot.getPromise(graphSnapshotAtom),
        ]);

        const updatedAt = new Date().toISOString();
        const payload: ProjectSnapshot = {
          path,
          meta,
          settings,
          graph,
          updatedAt,
        };
        const hash = serializeSnapshot(payload);

        try {
          await projectPersistence.save(payload);
          set(persistenceStatusAtom, (prev) => ({
            ...prev,
            isSaving: false,
            isDirty: false,
            lastSavedAt: updatedAt,
            lastSavedHash: hash,
            lastError: null,
          }));
        } catch (error) {
          set(persistenceStatusAtom, (prev) => ({
            ...prev,
            isSaving: false,
            lastError: error instanceof Error ? error.message : 'Failed to save project snapshot',
          }));
          throw error;
        }
      },
    [projectPath],
  );

  const loadProject = useCallback(
    (path: string) => {
      const trimmed = path?.trim();
      if (!trimmed) {
        throw new Error('Project path must not be empty');
      }
      selectProjectPath(trimmed);
      return trimmed;
    },
    [selectProjectPath],
  );

  const openProject = useCallback(async () => {
    const selected = await engine.browseForProject();
    if (!selected) return null;
    loadProject(selected);
    return selected;
  }, [loadProject]);

  const createProject = useCallback(async () => {
    const selectedFolder = await engine.browseForFolder();
    if (!selectedFolder) return null;
    const template = 'default';
    const payload: ProjectDTO = await engine.createProject({
      name: 'Untitled',
      location: selectedFolder,
      width: 1920,
      height: 1080,
      fps: 60,
      template,
      colorSpace: 'Rec.709',
    });

    await projectPersistence.save({
      path: payload.path ?? null,
      meta: {
        name: payload.name,
        version: payload.version,
        template: payload.template ?? null,
        lastOpened: new Date().toISOString(),
      },
      settings: payload.settings,
      graph: coerceGraphSnapshot(payload.graph),
      updatedAt: new Date().toISOString(),
    });
    selectProjectPath(payload.path ?? null);
    return payload;
  }, [selectProjectPath]);

  return { saveProject, openProject, createProject, loadProject };
}
