import { selector } from 'recoil';
import { projectPathAtom, projectMetaAtom, projectSettingsAtom, graphSnapshotAtom } from '@state/atoms/project';
import type { ProjectSnapshot } from '@state/types';

export const projectSnapshotSelector = selector<ProjectSnapshot>({
  key: 'project:snapshot',
  get: ({ get }) => ({
    path: get(projectPathAtom),
    meta: get(projectMetaAtom),
    settings: get(projectSettingsAtom),
    graph: get(graphSnapshotAtom),
  }),
});
