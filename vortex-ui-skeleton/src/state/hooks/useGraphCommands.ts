import { useRecoilCallback, useSetRecoilState } from 'recoil';
import { graphSnapshotAtom } from '@state/atoms/project';
import { selectedNodePtrAtom } from '@state/atoms/editor';
import type { GraphEdgeSnapshot, GraphNodeSnapshot, GraphSnapshot } from '@state/types';
import { computeNextNodePosition } from '@state/utils/graph';
import type { NodeHandle, ProjectCommand } from '@/app/services/ipc/projectCommands';
import { useProjectCommandDispatch } from './useProjectCommandDispatch';

type Point = { x: number; y: number };
type NodeReference = GraphNodeSnapshot | string | number | null | undefined;

const edgeKey = (edge: GraphEdgeSnapshot) => edge.id ?? `${edge.source}-${edge.target}`;

export function useGraphCommands() {
  const setSelectedPtr = useSetRecoilState(selectedNodePtrAtom);
  const dispatchCommands = useProjectCommandDispatch();

  const ensureHandle = (node: GraphNodeSnapshot) => ({
    ptr: node.ptr ?? undefined,
    id: node.id,
  });

  const findNodeById = (graph: GraphSnapshot, identifier: string | null | undefined) => {
    if (!identifier) return undefined;
    return graph.nodes?.find((node: GraphNodeSnapshot) => node.id === identifier || String(node.ptr) === identifier);
  };

  const resolveNodeReference = (graph: GraphSnapshot, reference: NodeReference) => {
    if (!reference) return undefined;
    if (typeof reference === 'object') {
      if (!reference) return undefined;
      if (reference.id) {
        const byId = findNodeById(graph, reference.id);
        if (byId) return byId;
      }
      if (reference.ptr != null) {
        return graph.nodes?.find((node) => node.ptr === reference.ptr);
      }
      return undefined;
    }

    const identifier = typeof reference === 'number' ? String(reference) : reference;
    return findNodeById(graph, identifier);
  };

  const createNode = useRecoilCallback(
    ({ snapshot }) =>
      async (type: string, position?: Point) => {
        if (!type?.length) {
          throw new Error('Node type must be provided');
        }
        const graph = await snapshot.getPromise(graphSnapshotAtom);
        const resolvedPosition = position ?? computeNextNodePosition(graph);
        const clientNodeId = `node-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 6)}`;

        const commands: ProjectCommand[] = [
          {
            kind: 'graph.node.create',
            payload: {
              type,
              label: type,
              position: resolvedPosition,
              clientNodeId,
            },
          },
        ];

        const refreshed = await dispatchCommands(commands);
        if (refreshed) {
          const created = refreshed.graph.nodes.find((node) => node.id === clientNodeId);
          if (created?.ptr) {
            setSelectedPtr(created.ptr);
            return created.ptr;
          }
        }

        return null;
      },
    [dispatchCommands, setSelectedPtr],
  );

  const removeNode = useRecoilCallback(
    ({ snapshot, set }) =>
      async (ptr: number | null | undefined) => {
        if (!ptr) return;

        const graph = await snapshot.getPromise(graphSnapshotAtom);
        const nodes = graph.nodes ?? [];
        const edges = graph.edges ?? [];

        const removedIds = new Set<string>();
        const ptrString = String(ptr);

        const nextNodes = nodes.filter((node) => {
          const match = node.ptr === ptr || node.id === ptrString;
          if (match) {
            removedIds.add(node.id);
          }
          return !match;
        });

        if (!removedIds.size) {
          removedIds.add(ptrString);
        }

        const nextEdges = edges.filter((edge) => !removedIds.has(edge.source) && !removedIds.has(edge.target));
        const targetNode = nodes.find((node) => node.ptr === ptr || node.id === ptrString);
        if (!targetNode) {
          return;
        }

        const commands: ProjectCommand[] = [
          {
            kind: 'graph.node.remove',
            payload: {
              target: ensureHandle(targetNode),
            },
          },
        ];

        await dispatchCommands(commands);
        set(graphSnapshotAtom, { nodes: nextNodes, edges: nextEdges });
        set(selectedNodePtrAtom, (current) => (current === ptr ? null : current));
      },
    [dispatchCommands],
  );

  const removeEdges = useRecoilCallback(
    ({ snapshot, set }) =>
      async (edgesToRemove: GraphEdgeSnapshot[] | null | undefined) => {
        const list = edgesToRemove?.filter(Boolean) as GraphEdgeSnapshot[] | undefined;
        if (!list || list.length === 0) return;

        const commandBatch: ProjectCommand[] = list.map((edge) => ({
          kind: 'graph.edge.disconnect',
          payload: {
            edgeId: edge.id,
            source: { id: edge.source },
            target: { id: edge.target },
          },
        }));

        await dispatchCommands(commandBatch, { reload: false });

        const graph = await snapshot.getPromise(graphSnapshotAtom);
        const removeIds = new Set(list.map(edgeKey));

        set(graphSnapshotAtom, {
          nodes: graph.nodes ?? [],
          edges: (graph.edges ?? []).filter((edge) => !removeIds.has(edgeKey(edge))),
        });
      },
    [dispatchCommands],
  );

  const connectNodes = useRecoilCallback(
    ({ snapshot }) =>
      async (connection: { source: NodeReference; target: NodeReference; sourceSlot?: number; targetSlot?: number }) => {
        const graph = await snapshot.getPromise(graphSnapshotAtom);
        const sourceNode = resolveNodeReference(graph, connection.source);
        const targetNode = resolveNodeReference(graph, connection.target);
        const sourceHandle = sourceNode ? ensureHandle(sourceNode) : handleFromReference(connection.source);
        const targetHandle = targetNode ? ensureHandle(targetNode) : handleFromReference(connection.target);
        if (!sourceHandle || !targetHandle) {
          throw new Error('Unable to resolve nodes for connection');
        }

        const commands: ProjectCommand[] = [
          {
            kind: 'graph.edge.connect',
            payload: {
              source: { ...sourceHandle, slot: connection.sourceSlot ?? 0 },
              target: { ...targetHandle, slot: connection.targetSlot ?? 0 },
            },
          },
        ];

        await dispatchCommands(commands, { reload: false });
      },
    [dispatchCommands],
  );

  const updateNodePosition = useRecoilCallback(
    ({ snapshot }) =>
      async (nodeId: string, position: Point) => {
        if (!nodeId) {
          return;
        }
        const graph = await snapshot.getPromise(graphSnapshotAtom);
        const node = findNodeById(graph, nodeId);
        if (!node) {
          return;
        }

        const commands: ProjectCommand[] = [
          {
            kind: 'graph.node.position',
            payload: {
              target: ensureHandle(node),
              position,
            },
          },
        ];

        await dispatchCommands(commands, { reload: false });
      },
    [dispatchCommands],
  );

  const updateNodeLabel = useRecoilCallback(
    ({ snapshot, set }) =>
      async (reference: NodeReference, label: string) => {
        if (!reference) {
          return;
        }

        const graph = await snapshot.getPromise(graphSnapshotAtom);
        const node = resolveNodeReference(graph, reference);
        if (!node) {
          return;
        }

        const normalized = label?.trim()?.length ? label.trim() : node.type ?? 'Node';
        const commands: ProjectCommand[] = [
          { kind: 'graph.node.label', payload: { target: ensureHandle(node), label: normalized } },
        ];

        await dispatchCommands(commands, { reload: false });

        set(graphSnapshotAtom, (prev) => ({
          ...prev,
          nodes: (prev.nodes ?? []).map((candidate) =>
            candidate.id === node.id || (node.ptr != null && candidate.ptr === node.ptr)
              ? { ...candidate, label: normalized }
              : candidate,
          ),
        }));
      },
    [dispatchCommands],
  );

  const updateNodeProps = useRecoilCallback(
    ({ snapshot, set }) =>
      async (reference: NodeReference, props: Record<string, unknown>) => {
        if (!reference) {
          return;
        }

        const entries = Object.entries(props ?? {}).filter(([key]) => typeof key === 'string');
        if (!entries.length) {
          return;
        }

        const graph = await snapshot.getPromise(graphSnapshotAtom);
        const node = resolveNodeReference(graph, reference);
        const handle = node ? ensureHandle(node) : handleFromReference(reference);
        if (!handle) {
          console.warn('[GraphCommands] Unable to resolve node for property update', reference);
          return;
        }

        const patch = Object.fromEntries(entries);
        const commands: ProjectCommand[] = [
          { kind: 'graph.node.props', payload: { target: handle, props: patch } },
        ];

        await dispatchCommands(commands, { reload: false });

        if (node) {
          set(graphSnapshotAtom, (prev) => ({
            ...prev,
            nodes: (prev.nodes ?? []).map((candidate) => {
              if (candidate.id !== node.id && (node.ptr == null || candidate.ptr !== node.ptr)) {
                return candidate;
              }
              const nextProps = { ...(candidate.props ?? {}), ...patch };
              return { ...candidate, props: nextProps };
            }),
          }));
          return;
        }

        const handlePtr = typeof handle.ptr === 'number' ? handle.ptr : undefined;
        const handleId = typeof handle.id === 'string' && handle.id.trim().length ? handle.id.trim() : undefined;
        if (!handlePtr && !handleId) {
          return;
        }

        set(graphSnapshotAtom, (prev) => {
          const nodes = prev.nodes ?? [];
          let updated = false;
          const nextNodes = nodes.map((candidate) => {
            if (!candidate) {
              return candidate;
            }
            const matchesPtr = typeof handlePtr === 'number' && candidate.ptr === handlePtr;
            const matchesId = typeof handleId === 'string' && candidate.id === handleId;
            if (!matchesPtr && !matchesId) {
              return candidate;
            }
            updated = true;
            const nextProps = { ...(candidate.props ?? {}), ...patch };
            return {
              ...candidate,
              props: nextProps,
              ptr: matchesPtr ? candidate.ptr : candidate.ptr ?? handlePtr,
            };
          });

          if (!updated) {
            return prev;
          }

          return { ...prev, nodes: nextNodes };
        });
      },
    [dispatchCommands],
  );

  return {
    createNode,
    removeNode,
    removeEdges,
    connectNodes,
    updateNodePosition,
    updateNodeLabel,
    updateNodeProps,
  };
}

function handleFromReference(reference: NodeReference): NodeHandle | null {
  if (!reference) {
    return null;
  }

  if (typeof reference === 'object') {
    const candidate = reference as GraphNodeSnapshot & { ptr?: number | null; id?: string | null };
    const ptr = Number(candidate.ptr);
    const hasPtr = Number.isFinite(ptr) && ptr > 0;
    const id = typeof candidate.id === 'string' && candidate.id.trim().length ? candidate.id.trim() : undefined;
    if (!hasPtr && !id) {
      return null;
    }
    const handle: NodeHandle = {};
    if (hasPtr) handle.ptr = ptr;
    if (id) handle.id = id;
    return handle;
  }

  if (typeof reference === 'number' && Number.isFinite(reference)) {
    return { ptr: Number(reference) };
  }

  if (typeof reference === 'string') {
    const trimmed = reference.trim();
    return trimmed.length ? { id: trimmed } : null;
  }

  return null;
}
