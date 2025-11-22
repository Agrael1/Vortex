import type { ProjectSnapshot } from '@state/types';

export const normalizeSnapshot = (snapshot: ProjectSnapshot | null): ProjectSnapshot | null => {
  if (!snapshot) return null;
  return {
    path: snapshot.path ?? null,
    meta: snapshot.meta,
    settings: snapshot.settings,
    graph: snapshot.graph,
  };
};

export const serializeSnapshot = (snapshot: ProjectSnapshot | null): string | null => {
  const normalized = normalizeSnapshot(snapshot);
  return normalized ? JSON.stringify(normalized) : null;
};

export const snapshotsEqual = (a: ProjectSnapshot | null, b: ProjectSnapshot | null) => {
  const left = serializeSnapshot(a);
  const right = serializeSnapshot(b);
  return left !== null && left === right;
};
