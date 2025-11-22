import { atom } from 'recoil';
import type { LoadSource } from '@services/persistence';

export type PersistenceStatus = {
  isHydrated: boolean;
  isSaving: boolean;
  isDirty: boolean;
  lastSavedAt: string | null;
  lastSavedHash: string | null;
  lastError: string | null;
  lastLoadSource: LoadSource | null;
};

export const persistenceStatusAtom = atom<PersistenceStatus>({
  key: 'persistence:status',
  default: {
    isHydrated: false,
    isSaving: false,
    isDirty: false,
    lastSavedAt: null,
    lastSavedHash: null,
    lastError: null,
    lastLoadSource: null,
  },
});
