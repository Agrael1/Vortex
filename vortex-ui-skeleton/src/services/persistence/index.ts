import { compare, type Operation } from 'fast-json-patch';
import type { ProjectSnapshot, GraphSnapshot, GraphNodeSnapshot, GraphEdgeSnapshot } from '@state/types';
import { engine, type ProjectDTO, type NodeDTO } from '@/app/services/ipc/cefBridge';

const cloneSnapshot = (snapshot: ProjectSnapshot): ProjectSnapshot => JSON.parse(JSON.stringify(snapshot));

type NativeProjectGateway = {
  openProject: (path: string) => Promise<ProjectDTO>;
  saveProject: (snapshot: ProjectSnapshot) => Promise<void>;
  applyPatch: (path: string, patch: Operation[]) => Promise<void>;
  replaceSnapshot: (snapshot: ProjectSnapshot) => Promise<void>;
  persistProject: (path: string) => Promise<void>;
};

const defaultNativeGateway: NativeProjectGateway = {
  openProject: (path: string) => engine.openProject(path),
  saveProject: async (snapshot: ProjectSnapshot) => {
    await engine.saveProject(snapshot);
  },
  applyPatch: async (path: string, patch: Operation[]) => {
    if (!patch.length) {
      return;
    }
    await engine.applyProjectPatch(path, patch);
  },
  replaceSnapshot: async (snapshot: ProjectSnapshot) => {
    await engine.resetProjectState(snapshot);
  },
  persistProject: async (path: string) => {
    await engine.persistProject(path);
  },
};

export type LoadSource = 'native';

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
        const dtoNode = node as NodeDTO;
        const ptrFromDto = Number(dtoNode.ptr);
        const ptrFromId = Number(node.id);
        const resolvedPtr = Number.isFinite(ptrFromDto) && ptrFromDto > 0
          ? ptrFromDto
          : Number.isFinite(ptrFromId) && ptrFromId > 0
            ? ptrFromId
            : undefined;
        return {
          id: typeof node.id === 'string' && node.id.length ? node.id : `node-${index}`,
          type: node.type ?? 'Node',
          label: (node.params as Record<string, unknown>)?.label as string | undefined,
          position: { x, y },
          ptr: resolvedPtr,
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
  private readonly syncedSnapshots = new Map<string, ProjectSnapshot>();

  constructor(private readonly gateway: NativeProjectGateway = defaultNativeGateway) {}

  private rememberSnapshot(snapshot: ProjectSnapshot) {
    if (!snapshot.path) {
      return;
    }

    this.syncedSnapshots.set(snapshot.path, cloneSnapshot(snapshot));
  }

  private async loadFromNative(path: string): Promise<ProjectSnapshot | null> {
    const nativeProject = await this.gateway.openProject(path);
    if (!nativeProject) {
      return null;
    }
    const snapshot = this.fromNativeProject(nativeProject);
    this.rememberSnapshot(snapshot);
    return snapshot;
  }

  async load(path: string | null): Promise<LoadResult | null> {
    if (!path) return null;

    const nativeSnapshot = await this.loadFromNative(path);
    if (!nativeSnapshot) {
      return null;
    }
    return { source: 'native', snapshot: nativeSnapshot };
  }

  async save(snapshot: ProjectSnapshot): Promise<void> {
    if (!snapshot.path) {
      throw new Error('Cannot save snapshot without a project path');
    }

    const stamped: ProjectSnapshot = {
      ...snapshot,
      updatedAt: snapshot.updatedAt ?? new Date().toISOString(),
    };

    const previous = this.syncedSnapshots.get(snapshot.path) ?? null;
    const patch = previous ? compare(previous, stamped) : null;
    const usedNative = await this.tryNativeSync(stamped, previous, patch);

    if (!usedNative) {
      await this.gateway.saveProject(stamped);
    }

    this.rememberSnapshot(stamped);
  }

  private async tryNativeSync(
    snapshot: ProjectSnapshot,
    previous: ProjectSnapshot | null,
    patch: Operation[] | null,
  ): Promise<boolean> {
    if (!snapshot.path) {
      return false;
    }

    try {
      if (!previous) {
        await this.gateway.replaceSnapshot(snapshot);
      } else if (patch && patch.length) {
        await this.gateway.applyPatch(snapshot.path, patch);
      }

      await this.gateway.persistProject(snapshot.path);
      return true;
    } catch (error) {
      console.warn('[Persistence] Native patch/persist failed, falling back to legacy save', error);
      return false;
    }
  }

  async refreshFromNative(path: string | null): Promise<ProjectSnapshot | null> {
    if (!path) return null;
    try {
      return await this.loadFromNative(path);
    } catch (error) {
      console.warn('[Persistence] Native refresh failed', error);
      return null;
    }
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

export const projectPersistence = new ProjectPersistence();
