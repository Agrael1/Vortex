import { useCallback } from 'react';
import { useRecoilValue, useSetRecoilState } from 'recoil';
import type { ProjectSnapshot } from '@state/types';
import { projectPathAtom } from '@state/atoms/project';
import { persistenceStatusAtom } from '@state/atoms/persistence';
import type { ProjectCommand } from '@/app/services/ipc/projectCommands';
import { submitProjectCommands } from '@services/commands';
import { usePersistenceActions } from './usePersistenceActions';

export type DispatchCommandsOptions = {
  reload?: boolean;
};

export function useProjectCommandDispatch() {
  const projectPath = useRecoilValue(projectPathAtom);
  const setStatus = useSetRecoilState(persistenceStatusAtom);
  const { reloadFromDisk } = usePersistenceActions();

  return useCallback(
    async (commands: ProjectCommand[], options?: DispatchCommandsOptions): Promise<ProjectSnapshot | null> => {
      if (!projectPath || !projectPath.trim().length) {
        throw new Error('No project path selected');
      }
      if (!commands.length) {
        return null;
      }

      setStatus((prev) => ({ ...prev, isSaving: true, isDirty: true, lastError: null }));

      try {
        await submitProjectCommands({
          projectPath,
          baseRevision: null,
          commands,
          source: 'ui',
        });
        setStatus((prev) => ({ ...prev, isSaving: false, isDirty: true }));
        if (options?.reload === false) {
          return null;
        }
        return await reloadFromDisk();
      } catch (error) {
        const message = error instanceof Error ? error.message : 'Failed to submit project commands';
        setStatus((prev) => ({ ...prev, isSaving: false, lastError: message }));
        if (options?.reload !== false) {
          try {
            await reloadFromDisk();
          } catch (reloadError) {
            console.warn('[Commands] Reload after failed submission also failed', reloadError);
          }
        }
        throw error;
      }
    },
    [projectPath, reloadFromDisk, setStatus],
  );
}
