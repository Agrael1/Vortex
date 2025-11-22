import { useRecoilCallback, useSetRecoilState } from 'recoil';
import { graphSnapshotAtom } from '@state/atoms/project';
import { selectedNodePtrAtom } from '@state/atoms/editor';
import type { GraphEdgeSnapshot, GraphNodeSnapshot, GraphSnapshot } from '@state/types';
import { computeNextNodePosition } from '@state/utils/graph';
import { Vortex } from '@/bridge/vortex';

type Point = { x: number; y: number };

const edgeKey = (edge: GraphEdgeSnapshot) => edge.id ?? `${edge.source}-${edge.target}`;

export function useGraphCommands() {
  const setSelectedPtr = useSetRecoilState(selectedNodePtrAtom);

  const createNode = useRecoilCallback(
    ({ snapshot, set }) =>
      async (type: string, position?: Point) => {
        if (!type?.length) {
          throw new Error('Node type must be provided');
        }

        const ptr = await Vortex.createNode(type);
        const graph = await snapshot.getPromise(graphSnapshotAtom);
        const resolvedPosition = position ?? computeNextNodePosition(graph);
        const nodeId = String(ptr ?? `${type}-${Date.now()}`);

        const nextNode: GraphNodeSnapshot = {
          id: nodeId,
          type,
          label: type,
          position: resolvedPosition,
          ptr,
          props: {},
        };

        const nextGraph: GraphSnapshot = {
          nodes: [...(graph.nodes ?? []), nextNode],
          edges: graph.edges ?? [],
        };

        set(graphSnapshotAtom, nextGraph);
        setSelectedPtr(ptr);

        return ptr;
      },
    [setSelectedPtr],
  );

  const removeNode = useRecoilCallback(
    ({ snapshot, set }) =>
      async (ptr: number | null | undefined) => {
        if (!ptr) return;

        try {
          await Vortex.removeNode(ptr);
        } catch (error) {
          console.warn('[GraphCommands] Failed to remove node in engine', error);
        }

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

        set(graphSnapshotAtom, { nodes: nextNodes, edges: nextEdges });
        set(selectedNodePtrAtom, (current) => (current === ptr ? null : current));
      },
    [],
  );

  const removeEdges = useRecoilCallback(
    ({ snapshot, set }) =>
      async (edgesToRemove: GraphEdgeSnapshot[] | null | undefined) => {
        const list = edgesToRemove?.filter(Boolean) as GraphEdgeSnapshot[] | undefined;
        if (!list || list.length === 0) return;

        await Promise.all(
          list.map(async (edge) => {
            const srcPtr = Number(edge.source);
            const dstPtr = Number(edge.target);
            if (!Number.isFinite(srcPtr) || !Number.isFinite(dstPtr)) return;
            try {
              await Vortex.disconnect(srcPtr, 0, dstPtr, 0);
            } catch (error) {
              console.warn('[GraphCommands] Failed to disconnect edge', edge, error);
            }
          }),
        );

        const graph = await snapshot.getPromise(graphSnapshotAtom);
        const removeIds = new Set(list.map(edgeKey));

        set(graphSnapshotAtom, {
          nodes: graph.nodes ?? [],
          edges: (graph.edges ?? []).filter((edge) => !removeIds.has(edgeKey(edge))),
        });
      },
    [],
  );

  return { createNode, removeNode, removeEdges };
}
