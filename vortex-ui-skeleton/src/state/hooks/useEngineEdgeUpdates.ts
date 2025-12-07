import { useEffect } from 'react';
import { useSetRecoilState } from 'recoil';
import { graphSnapshotAtom } from '@state/atoms/project';
import type { GraphEdgeSnapshot, GraphNodeSnapshot, GraphSnapshot } from '@state/types';
import { engine, type EdgeConnectionPayload, type EdgeDisconnectionPayload } from '@/app/services/ipc/cefBridge';

const toNodeId = (nodes: GraphNodeSnapshot[] | undefined, ptr: number | undefined): string | null => {
  if (!Number.isFinite(ptr)) {
    return null;
  }

  const match = nodes?.find((node) => node.ptr === ptr);
  if (match?.id) {
    return match.id;
  }

  return String(ptr);
};

const safeSlot = (value: number | undefined) => (Number.isFinite(value) ? Number(value) : 0);

const normalizeEdgeId = (
  payloadId: string | null | undefined,
  sourceId: string,
  targetId: string,
  sourceSlot?: number,
  targetSlot?: number,
) => {
  if (typeof payloadId === 'string' && payloadId.trim().length) {
    return payloadId.trim();
  }
  return `${sourceId}:${safeSlot(sourceSlot)}->${targetId}:${safeSlot(targetSlot)}`;
};

const edgeMatchesPayload = (
  edge: GraphEdgeSnapshot,
  payload: EdgeDisconnectionPayload,
  nodes: GraphNodeSnapshot[] | undefined,
): boolean => {
  if (payload.edgeId && edge.id === payload.edgeId) {
    return true;
  }

  const expectedSource = payload.sourcePtr != null ? toNodeId(nodes, payload.sourcePtr) : null;
  const expectedTarget = payload.targetPtr != null ? toNodeId(nodes, payload.targetPtr) : null;
  const expectedSourceSlot = payload.sourceSlot;
  const expectedTargetSlot = payload.targetSlot;

  const slotMatches = (edgeSlot: number | undefined, expectedSlot: number | undefined) => {
    if (expectedSlot == null) {
      return true;
    }
    return safeSlot(edgeSlot) === safeSlot(expectedSlot);
  };

  if (expectedSource && expectedTarget) {
    return (
      edge.source === expectedSource &&
      edge.target === expectedTarget &&
      slotMatches(edge.sourceSlot, expectedSourceSlot) &&
      slotMatches(edge.targetSlot, expectedTargetSlot)
    );
  }

  if (expectedSource) {
    return edge.source === expectedSource && slotMatches(edge.sourceSlot, expectedSourceSlot);
  }

  if (expectedTarget) {
    return edge.target === expectedTarget && slotMatches(edge.targetSlot, expectedTargetSlot);
  }

  return false;
};

const edgeAlreadyExists = (
  edges: GraphEdgeSnapshot[] | undefined,
  candidateId: string,
  sourceId: string,
  targetId: string,
  sourceSlot?: number,
  targetSlot?: number,
) => {
  if (!edges?.length) {
    return false;
  }

  return edges.some((edge) => {
    if (edge.id && candidateId) {
      return edge.id === candidateId;
    }
    return (
      edge.source === sourceId &&
      edge.target === targetId &&
      safeSlot(edge.sourceSlot) === safeSlot(sourceSlot) &&
      safeSlot(edge.targetSlot) === safeSlot(targetSlot)
    );
  });
};

export function useEngineEdgeUpdates() {
  const setGraphSnapshot = useSetRecoilState(graphSnapshotAtom);

  useEffect(() => {
    const offConnect = engine.on<EdgeConnectionPayload>('edge:connected', (payload) => {
      if (!payload) return;
      setGraphSnapshot((prev) => {
        const nodes = prev.nodes ?? [];
        const edges = prev.edges ?? [];
        const sourceId = toNodeId(nodes, payload.sourcePtr);
        const targetId = toNodeId(nodes, payload.targetPtr);
        if (!sourceId || !targetId) {
          return prev;
        }

        const edgeId = normalizeEdgeId(payload.edgeId, sourceId, targetId, payload.sourceSlot, payload.targetSlot);
        if (edgeAlreadyExists(edges, edgeId, sourceId, targetId, payload.sourceSlot, payload.targetSlot)) {
          return prev;
        }

        const nextEdge: GraphEdgeSnapshot = {
          id: edgeId,
          source: sourceId,
          target: targetId,
          animated: true,
          sourceSlot: payload.sourceSlot,
          targetSlot: payload.targetSlot,
        };

        const nextSnapshot: GraphSnapshot = {
          nodes,
          edges: [...edges, nextEdge],
        };

        return nextSnapshot;
      });
    });

    const offDisconnect = engine.on<EdgeDisconnectionPayload>('edge:disconnected', (payload) => {
      if (!payload) return;
      setGraphSnapshot((prev) => {
        const edges = prev.edges ?? [];
        if (!edges.length) {
          return prev;
        }

        const nodes = prev.nodes ?? [];
        const filtered = edges.filter((edge) => !edgeMatchesPayload(edge, payload, nodes));
        if (filtered.length === edges.length) {
          return prev;
        }

        const nextSnapshot: GraphSnapshot = {
          nodes,
          edges: filtered,
        };

        return nextSnapshot;
      });
    });

    return () => {
      offConnect?.();
      offDisconnect?.();
    };
  }, [setGraphSnapshot]);
}
