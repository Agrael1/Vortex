import { atom } from 'recoil';
import type { ProjectPath, ProjectMeta, ProjectSettings, GraphSnapshot } from '@state/types';

export const projectPathAtom = atom<ProjectPath>({
  key: 'project:path',
  default: null,
});

export const projectMetaAtom = atom<ProjectMeta>({
  key: 'project:meta',
  default: {
    name: 'Untitled',
    version: '1.0.0',
  },
});

export const projectSettingsAtom = atom<ProjectSettings>({
  key: 'project:settings',
  default: {
    width: 1920,
    height: 1080,
    fps: 60,
    colorSpace: 'Rec.709',
  },
});

export const graphSnapshotAtom = atom<GraphSnapshot>({
  key: 'project:graph',
  default: { nodes: [], edges: [] },
});
