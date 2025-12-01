import { atom } from 'recoil';
import type { LoadSource } from '@services/persistence';

export type PersistenceStatus = {
  isHydrated: boolean;
  isSaving: boolean;
  isDirty: boolean;
  isReloading: boolean;
  lastSavedAt: string | null;
  lastSavedHash: string | null;
  lastError: string | null;
  lastLoadSource: LoadSource | null;
  lastPersistReason: string | null;
  hasExternalChange: boolean;
  lastExternalChangeAt: string | null;
  lastMutationAt: number | null;
};

export const persistenceStatusAtom = atom<PersistenceStatus>({
  key: 'persistence:status',
  default: {
    isHydrated: false,
    isSaving: false,
    isDirty: false,
    isReloading: false,
    lastSavedAt: null,
    lastSavedHash: null,
    lastError: null,
    lastLoadSource: null,
    lastPersistReason: null,
    hasExternalChange: false,
    lastExternalChangeAt: null,
    lastMutationAt: null,
  },
});
