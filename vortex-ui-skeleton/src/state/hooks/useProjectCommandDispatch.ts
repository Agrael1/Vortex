import { useCallback } from 'react';
import { useRecoilValue, useSetRecoilState } from 'recoil';
import type { ProjectSnapshot } from '@state/types';
import { projectPathAtom } from '@state/atoms/project';
import { persistenceStatusAtom } from '@state/atoms/persistence';
import type { CommandKind, ProjectCommand } from '@/app/services/ipc/projectCommands';
import { submitProjectCommands } from '@services/commands';
import { usePersistenceActions } from './usePersistenceActions';

export type DispatchCommandsOptions = {
  reload?: boolean;
};

const STRUCTURAL_COMMANDS = new Set<CommandKind>([
  'graph.node.create',
  'graph.node.remove',
  'project.settings',
  'project.meta',
]);

const shouldReloadAfter = (commands: ProjectCommand[], explicit?: boolean) => {
  if (typeof explicit === 'boolean') {
    return explicit;
  }
  return commands.some((command) => STRUCTURAL_COMMANDS.has(command.kind));
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

      const mutationStamp = Date.now();
      setStatus((prev) => ({ ...prev, isSaving: true, isDirty: true, lastError: null, lastMutationAt: mutationStamp }));

      try {
        await submitProjectCommands({
          projectPath,
          baseRevision: null,
          commands,
          source: 'ui',
        });
        setStatus((prev) => ({ ...prev, isSaving: false, isDirty: true, lastMutationAt: mutationStamp }));
        const reloadPreference = shouldReloadAfter(commands, options?.reload);
        if (process.env.NODE_ENV !== 'production') {
          console.debug('[CommandDispatch] Submitted commands', commands.map((command) => command.kind), {
            reloadPreference,
            explicit: options?.reload,
          });
        }
        if (!reloadPreference) {
          return null;
        }
        if (process.env.NODE_ENV !== 'production') {
          console.debug('[CommandDispatch] Reloading from disk after commands');
        }
        return await reloadFromDisk();
      } catch (error) {
        const message = error instanceof Error ? error.message : 'Failed to submit project commands';
        setStatus((prev) => ({ ...prev, isSaving: false, lastError: message, lastMutationAt: mutationStamp }));
        const shouldReload = shouldReloadAfter(commands, options?.reload);
        if (process.env.NODE_ENV !== 'production') {
          console.debug('[CommandDispatch] Submission failed, reload scheduled?', shouldReload, {
            commands: commands.map((command) => command.kind),
            explicit: options?.reload,
          });
        }
        if (shouldReload) {
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
