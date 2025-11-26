import type { ProjectCommand } from '@/app/services/ipc/projectCommands';
import { engine } from '@/app/services/ipc/cefBridge';

export type SubmitProjectCommandsOptions = {
  projectPath: string | null | undefined;
  baseRevision?: string | null;
  source?: string | null;
  commands: ProjectCommand[];
};

export const submitProjectCommands = async (options: SubmitProjectCommandsOptions): Promise<void> => {
  const { projectPath, commands, baseRevision = null, source = 'ui' } = options;
  if (!projectPath || !projectPath.trim().length) {
    throw new Error('Project path is required to submit commands');
  }
  if (!commands.length) {
    return;
  }

  await engine.submitProjectCommands({
    projectPath,
    baseRevision,
    source,
    commands,
  });
};
