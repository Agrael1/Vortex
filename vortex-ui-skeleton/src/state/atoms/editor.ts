import { atom } from 'recoil';
import type { ConsoleEntry } from '@state/types';
import type { NodePropertyCache } from '@/types/properties';

export const selectedNodePtrAtom = atom<number | null>({
  key: 'editor:selectedNodePtr',
  default: null,
});

export type SelectedNodeIdentity = {
  id: string | null;
  uid: string | null;
  ptr: number | null;
};

export const selectedNodeIdentityAtom = atom<SelectedNodeIdentity>({
  key: 'editor:selectedNodeIdentity',
  default: { id: null, uid: null, ptr: null },
});

export const consoleEntriesAtom = atom<ConsoleEntry[]>({
  key: 'editor:consoleEntries',
  default: [],
});

export const nodePropertyCacheAtom = atom<NodePropertyCache>({
  key: 'editor:nodePropertyCache',
  default: {},
});

export const nodePtrByIdAtom = atom<Record<string, number>>({
  key: 'editor:nodePtrById',
  default: {},
});

export const nodePtrByUidAtom = atom<Record<string, number>>({
  key: 'editor:nodePtrByUid',
  default: {},
});
