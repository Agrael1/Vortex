import { atom, type AtomEffect } from 'recoil';

export const AUTOSAVE_STORAGE_KEY = 'vortex.editor.autosave';

const autosaveStorageEffect: AtomEffect<boolean> = ({ setSelf, onSet }) => {
  try {
    const stored = window.localStorage.getItem(AUTOSAVE_STORAGE_KEY);
    if (stored !== null) {
      setSelf(stored === 'true');
    }
  } catch {
    /* ignore storage read errors */
  }

  onSet((value) => {
    try {
      window.localStorage.setItem(AUTOSAVE_STORAGE_KEY, value ? 'true' : 'false');
    } catch {
      /* ignore storage write errors */
    }
  });
};

export const autosavePreferenceAtom = atom<boolean>({
  key: 'preferences:autosave',
  default: true,
  effects_UNSTABLE: typeof window === 'undefined' ? [] : [autosaveStorageEffect],
});
