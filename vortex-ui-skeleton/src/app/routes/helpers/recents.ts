import type { Recent } from '@/app/services/ipc/cefBridge';
import type { ProjectSnapshot } from '@state/types';

export const RECENTS_STORAGE_KEY = 'vortex.hub.recents';
export const MAX_RECENTS = 30;

export const deriveNameFromPath = (path: string) => {
  if (!path) return 'Untitled';
  const normalized = path.replace(/\\/g, '/').split('/');
  const last = normalized[normalized.length - 1] || path;
  return last.replace(/\.[^.]+$/, '') || last;
};

export const formatLastUsed = (value: string) => {
  const ts = Date.parse(value);
  if (Number.isNaN(ts)) return 'Unknown date';

  const diff = Date.now() - ts;
  const minutes = Math.round(diff / 60000);
  if (minutes <= 1) return 'just now';
  if (minutes < 60) return `${minutes} min ago`;

  const hours = Math.round(minutes / 60);
  if (hours < 24) return `${hours} h ago`;

  const days = Math.round(hours / 24);
  if (days < 7) return `${days} d ago`;

  return new Date(ts).toLocaleDateString();
};

export const compareRecents = (a: Recent, b: Recent) => {
  const pinDiff = Number(Boolean(b.pinned)) - Number(Boolean(a.pinned));
  if (pinDiff !== 0) return pinDiff;

  const timeA = Date.parse(a.last);
  const timeB = Date.parse(b.last);
  if (!Number.isNaN(timeA) && !Number.isNaN(timeB) && timeA !== timeB) {
    return timeB - timeA;
  }

  return a.name.localeCompare(b.name);
};

export const loadRecentsFromStorage = (): Recent[] => {
  if (typeof window === 'undefined' || !('localStorage' in window)) return [];
  try {
    const raw = window.localStorage.getItem(RECENTS_STORAGE_KEY);
    if (!raw) return [];
    const parsed = JSON.parse(raw);
    if (!Array.isArray(parsed)) return [];
    const normalized = parsed
      .map((item) => {
        if (!item || typeof item !== 'object') return null;
        const rawPath = (item as any).path;
        if (typeof rawPath !== 'string' || !rawPath.length) return null;
        const nameValue = (item as any).name;
        const lastValue = (item as any).last;
        const name = typeof nameValue === 'string' && nameValue.trim().length ? nameValue.trim() : deriveNameFromPath(rawPath);
        const last = typeof lastValue === 'string' && lastValue.trim().length ? lastValue : new Date().toISOString();
        const template = typeof (item as any).template === 'string' ? (item as any).template : null;
        const width = Number.isFinite((item as any).width) ? Number((item as any).width) : undefined;
        const height = Number.isFinite((item as any).height) ? Number((item as any).height) : undefined;
        const fps = Number.isFinite((item as any).fps) ? Number((item as any).fps) : undefined;
        const colorSpace = typeof (item as any).colorSpace === 'string' ? (item as any).colorSpace : undefined;
        const preview = typeof (item as any).preview === 'string' ? (item as any).preview : null;
        const pinned = (item as any).pinned === true;
        const error = typeof (item as any).error === 'string' ? (item as any).error : null;

        const result: Recent = {
          name,
          path: rawPath,
          last,
          template,
          width,
          height,
          fps,
          colorSpace,
          preview,
          error,
        };

        if (pinned) {
          result.pinned = true;
        }

        return result;
      })
      .filter((item): item is Recent => Boolean(item));

    return normalized.sort(compareRecents);
  } catch {
    return [];
  }
};

export const persistRecents = (recents: Recent[]) => {
  if (typeof window === 'undefined' || !('localStorage' in window)) return;
  try {
    const ordered = [...recents].sort(compareRecents);
    const limited = ordered.slice(0, MAX_RECENTS);
    window.localStorage.setItem(RECENTS_STORAGE_KEY, JSON.stringify(limited));
  } catch {
    /* ignore storage write errors */
  }
};

export const mapSnapshotToRecent = (snapshot: ProjectSnapshot): Recent | null => {
  if (!snapshot.path) return null;

  const metaName = snapshot.meta?.name;
  const name = metaName?.trim()?.length ? metaName.trim() : deriveNameFromPath(snapshot.path);
  const last = snapshot.meta?.lastOpened ?? snapshot.updatedAt ?? new Date().toISOString();
  const settings = snapshot.settings ?? null;

  return {
    name,
    path: snapshot.path,
    last,
    template: snapshot.meta?.template ?? null,
    width: settings?.width,
    height: settings?.height,
    fps: settings?.fps,
    colorSpace: settings?.colorSpace,
  };
};

export const mergeRecentCollections = (...collections: Recent[][]): Recent[] => {
  const map = new Map<string, Recent>();
  collections.forEach((list) => {
    list.forEach((item) => {
      if (!item?.path) return;
      const previous = map.get(item.path);
      const merged: Recent = {
        ...previous,
        ...item,
        name: item.name?.trim()?.length ? item.name : previous?.name ?? deriveNameFromPath(item.path),
        last: item.last ?? previous?.last ?? new Date().toISOString(),
        pinned: previous?.pinned ?? item.pinned,
        template: item.template ?? previous?.template ?? null,
        preview: item.preview ?? previous?.preview ?? null,
        error: item.error ?? previous?.error ?? null,
      };
      map.set(item.path, merged);
    });
  });

  return Array.from(map.values()).sort(compareRecents);
};
