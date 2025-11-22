import localforage from 'localforage';
import type { ProjectSnapshot, GraphSnapshot, GraphNodeSnapshot, GraphEdgeSnapshot } from '@state/types';
import { engine, type ProjectDTO } from '@/app/services/ipc/cefBridge';

export interface SnapshotStore {
  load(path: string | null): Promise<ProjectSnapshot | null>;
  save(snapshot: ProjectSnapshot): Promise<void>;
  listRecent(): Promise<ProjectSnapshot[]>;
}

export class MemorySnapshotStore implements SnapshotStore {
  private readonly store = new Map<string, ProjectSnapshot>();

  async load(path: string | null): Promise<ProjectSnapshot | null> {
    if (!path) return null;
    return this.store.get(path) ?? null;
  }

  async save(snapshot: ProjectSnapshot): Promise<void> {
    if (!snapshot.path) return;
    this.store.set(snapshot.path, snapshot);
  }

  async listRecent(): Promise<ProjectSnapshot[]> {
    return Array.from(this.store.values());
  }
}

type SnapshotRecord = ProjectSnapshot & { updatedAt: string };

const SNAPSHOT_PREFIX = 'vortex.snapshot:';
const RECENTS_KEY = 'vortex.snapshot:recents';
const MAX_RECENT = 20;

const makeSnapshotKey = (path: string) => `${SNAPSHOT_PREFIX}${encodeURIComponent(path)}`;

export type LocalSnapshotStoreOptions = {
  name?: string;
  storeName?: string;
  recentsKey?: string;
  maxRecent?: number;
};

export class LocalSnapshotStore implements SnapshotStore {
  private readonly driver: ReturnType<typeof localforage.createInstance>;
  private readonly recentsKey: string;
  private readonly maxRecent: number;

  constructor(options?: LocalSnapshotStoreOptions) {
    this.driver = localforage.createInstance({
      name: options?.name ?? 'vortex-ui',
      storeName: options?.storeName ?? 'snapshots',
      description: 'Vortex UI project snapshots',
    });

    this.recentsKey = options?.recentsKey ?? RECENTS_KEY;
    this.maxRecent = options?.maxRecent ?? MAX_RECENT;
  }

  async load(path: string | null): Promise<ProjectSnapshot | null> {
    if (!path) return null;
    const record = await this.driver.getItem<SnapshotRecord>(makeSnapshotKey(path));
    return record ?? null;
  }

  async save(snapshot: ProjectSnapshot): Promise<void> {
    if (!snapshot.path) return;
    const record: SnapshotRecord = {
      ...snapshot,
      updatedAt: snapshot.updatedAt ?? new Date().toISOString(),
    };

    await this.driver.setItem(makeSnapshotKey(snapshot.path), record);
    await this.touchRecents(record);
  }

  async listRecent(): Promise<ProjectSnapshot[]> {
    const items = await this.driver.getItem<SnapshotRecord[]>(this.recentsKey);
    return Array.isArray(items) ? items : [];
  }

  private async touchRecents(record: SnapshotRecord) {
    const existing = (await this.driver.getItem<SnapshotRecord[]>(this.recentsKey)) ?? [];
    const merged = [record, ...existing.filter((item: SnapshotRecord) => item.path !== record.path)].slice(0, this.maxRecent);
    await this.driver.setItem(this.recentsKey, merged);
  }
}

type NativeProjectGateway = {
  openProject: (path: string) => Promise<ProjectDTO>;
  saveProject: (snapshot: ProjectSnapshot) => Promise<void>;
};

const defaultNativeGateway: NativeProjectGateway = {
  openProject: (path: string) => engine.openProject(path),
  saveProject: async () => {
    await engine.saveProject();
  },
};

export type LoadSource = 'native' | 'cache';

export type LoadResult = {
  source: LoadSource;
  snapshot: ProjectSnapshot;
};

export const coerceGraphSnapshot = (graph: ProjectDTO['graph'] | GraphSnapshot | undefined): GraphSnapshot => {
  if (!graph) {
    return { nodes: [], edges: [] };
  }

  const asGraphSnapshot = graph as GraphSnapshot;
  if (
    Array.isArray(asGraphSnapshot.nodes) &&
    Array.isArray(asGraphSnapshot.edges) &&
    asGraphSnapshot.nodes.every((node) => node && typeof node.position === 'object')
  ) {
    return {
      nodes: asGraphSnapshot.nodes.map((node) => ({
        id: node.id,
        type: node.type,
        label: node.label,
        position: node.position,
        ptr: node.ptr,
        props: node.props ? { ...node.props } : undefined,
      })),
      edges: asGraphSnapshot.edges.map((edge) => ({
        id: edge.id,
        source: edge.source,
        target: edge.target,
        animated: edge.animated,
      })),
    };
  }

  const nativeGraph = graph as ProjectDTO['graph'];
  const nodes: GraphNodeSnapshot[] = Array.isArray(nativeGraph?.nodes)
    ? nativeGraph.nodes.map((node, index) => {
        const x = Array.isArray(node.pos) && typeof node.pos[0] === 'number' ? node.pos[0] : 0;
        const y = Array.isArray(node.pos) && typeof node.pos[1] === 'number' ? node.pos[1] : 0;
        const ptrCandidate = Number(node.id);
        return {
          id: typeof node.id === 'string' && node.id.length ? node.id : `node-${index}`,
          type: node.type ?? 'Node',
          label: (node.params as Record<string, unknown>)?.label as string | undefined,
          position: { x, y },
          ptr: Number.isFinite(ptrCandidate) ? ptrCandidate : undefined,
          props: typeof node.params === 'object' && node.params != null ? { ...(node.params as Record<string, unknown>) } : undefined,
        };
      })
    : [];

  const edges: GraphEdgeSnapshot[] = Array.isArray(nativeGraph?.edges)
    ? nativeGraph.edges.map((edge, index) => ({
        id: edge.id ?? `edge-${index}`,
        source: edge.from,
        target: edge.to,
        animated: true,
      }))
    : [];

  return { nodes, edges };
};

export class ProjectPersistence {
  constructor(
    private readonly store: SnapshotStore,
    private readonly gateway: NativeProjectGateway = defaultNativeGateway,
  ) {}

  async load(path: string | null): Promise<LoadResult | null> {
    if (!path) return null;

    const cached = await this.store.load(path);
    if (cached) {
      return { source: 'cache', snapshot: cached };
    }

    const nativeProject = await this.gateway.openProject(path);
    const snapshot = this.fromNativeProject(nativeProject);
    await this.store.save(snapshot);
    return { source: 'native', snapshot };
  }

  async save(snapshot: ProjectSnapshot): Promise<void> {
    if (!snapshot.path) {
      throw new Error('Cannot save snapshot without a project path');
    }

    await this.store.save({ ...snapshot, updatedAt: new Date().toISOString() });
    await this.gateway.saveProject(snapshot);
  }

  async listRecent(): Promise<ProjectSnapshot[]> {
    return this.store.listRecent();
  }

  fromNativeProject(dto: ProjectDTO): ProjectSnapshot {
    const metaVersion = dto.version || '1.0';
    return {
      path: dto.path ?? null,
      meta: {
        name: dto.name ?? 'Untitled',
        version: metaVersion,
        template: dto.template ?? null,
        lastOpened: new Date().toISOString(),
      },
      settings: dto.settings,
      graph: coerceGraphSnapshot(dto.graph),
      updatedAt: new Date().toISOString(),
    };
  }
}

const defaultStore = new LocalSnapshotStore();
export const projectPersistence = new ProjectPersistence(defaultStore);
