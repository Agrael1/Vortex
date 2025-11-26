import type { Recent } from '@/app/services/ipc/cefBridge';
import { MAX_RECENTS, compareRecents, deriveNameFromPath, mergeRecentCollections } from './recents';

export type RecentsAction =
  | { type: 'hydrate'; payload: Recent[] }
  | { type: 'update'; entry: Partial<Recent> & { path: string } }
  | { type: 'remove'; path: string }
  | { type: 'togglePin'; path: string }
  | { type: 'merge'; collections: Recent[][] };

export const sortAndLimitRecents = (recents: Recent[]) => {
  if (recents.length === 0) return [] as Recent[];
  const ordered = [...recents].sort(compareRecents);
  return ordered.length > MAX_RECENTS ? ordered.slice(0, MAX_RECENTS) : ordered;
};

const normalizeRecent = (entry: Partial<Recent> & { path: string }, existing?: Recent): Recent => {
  const name = entry.name?.trim()?.length ? entry.name.trim() : existing?.name ?? deriveNameFromPath(entry.path);
  const last = entry.last ?? existing?.last ?? new Date().toISOString();
  const width = entry.width ?? existing?.width;
  const height = entry.height ?? existing?.height;
  const fps = entry.fps ?? existing?.fps;
  const colorSpace = entry.colorSpace ?? existing?.colorSpace;
  const template = entry.template ?? existing?.template ?? null;
  const preview = entry.preview ?? existing?.preview ?? null;
  const error = entry.error ?? existing?.error ?? null;
  const pinned = entry.pinned ?? existing?.pinned;

  const result: Recent = {
    ...existing,
    ...entry,
    name,
    path: entry.path,
    last,
    width,
    height,
    fps,
    colorSpace,
    template,
    preview,
    error,
  };

  if (pinned) {
    result.pinned = true;
  } else {
    delete result.pinned;
  }

  return result;
};

export const recentsReducer = (state: Recent[], action: RecentsAction): Recent[] => {
  switch (action.type) {
    case 'hydrate': {
      return sortAndLimitRecents(action.payload ?? []);
    }
    case 'update': {
      const nextRecent = normalizeRecent(action.entry, state.find((item) => item.path === action.entry.path));
      const filtered = state.filter((item) => item.path !== action.entry.path);
      return sortAndLimitRecents([nextRecent, ...filtered]);
    }
    case 'remove': {
      const filtered = state.filter((item) => item.path !== action.path);
      if (filtered.length === state.length) {
        return state;
      }
      return sortAndLimitRecents(filtered);
    }
    case 'togglePin': {
      let touched = false;
      const flipped = state.map((item) => {
        if (item.path !== action.path) return item;
        touched = true;
        if (item.pinned) {
          const rest = { ...item };
          delete rest.pinned;
          return rest;
        }
        return { ...item, pinned: true };
      });
      if (!touched) {
        return state;
      }
      return sortAndLimitRecents(flipped);
    }
    case 'merge': {
      if (!action.collections?.length) {
        return state;
      }
      const merged = mergeRecentCollections(state, ...action.collections);
      return sortAndLimitRecents(merged);
    }
    default:
      return state;
  }
};
