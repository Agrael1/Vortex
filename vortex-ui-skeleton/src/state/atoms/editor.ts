import { atom } from 'recoil';
import type { ConsoleEntry } from '@state/types';

export const selectedNodePtrAtom = atom<number | null>({
  key: 'editor:selectedNodePtr',
  default: null,
});

export const consoleEntriesAtom = atom<ConsoleEntry[]>({
  key: 'editor:consoleEntries',
  default: [],
});
