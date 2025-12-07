import React from 'react';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import { render, screen, waitFor } from '@testing-library/react';
import { act } from 'react';
import { RecoilRoot, useRecoilValue } from 'recoil';
import { graphSnapshotAtom } from '@state/atoms/project';
import { useEngineEdgeUpdates } from '@state/hooks/useEngineEdgeUpdates';
import type { GraphSnapshot } from '@state/types';

vi.mock('@/app/services/ipc/cefBridge', () => {
  const listeners = new Map<string, Set<(payload: unknown) => void>>();
  const on = (event: string, handler: (payload: unknown) => void) => {
    const bucket = listeners.get(event) ?? new Set();
    bucket.add(handler);
    listeners.set(event, bucket);
    return () => {
      bucket.delete(handler);
      if (!bucket.size) {
        listeners.delete(event);
      }
    };
  };
  const emit = (event: string, payload: unknown) => {
    listeners.get(event)?.forEach((handler) => handler(payload));
  };
  const reset = () => listeners.clear();
  return {
    engine: {
      on,
      __emit: emit,
      __reset: reset,
    },
  };
});

import { engine } from '@/app/services/ipc/cefBridge';

type EngineMock = typeof engine & {
  __emit?: (event: string, payload: unknown) => void;
  __reset?: () => void;
};

const getEngineMock = () => engine as EngineMock;

const GraphObserver = () => {
  useEngineEdgeUpdates();
  const graph = useRecoilValue(graphSnapshotAtom);
  return <pre data-testid="graph-state">{JSON.stringify(graph)}</pre>;
};

const renderWithGraph = (graph: GraphSnapshot) => {
  return render(
    <RecoilRoot
      initializeState={({ set }) => {
        set(graphSnapshotAtom, graph);
      }}
    >
      <GraphObserver />
    </RecoilRoot>,
  );
};

const baseGraph: GraphSnapshot = {
  nodes: [
    { id: 'node-100', type: 'Node', label: 'Alpha', position: { x: 0, y: 0 }, ptr: 100 },
    { id: 'node-200', type: 'Node', label: 'Beta', position: { x: 200, y: 0 }, ptr: 200 },
  ],
  edges: [],
};

describe('useEngineEdgeUpdates', () => {
  beforeEach(() => {
    getEngineMock().__reset?.();
  });

  it('adds a new edge when engine reports a connection', async () => {
    renderWithGraph(baseGraph);

    await act(async () => {
      getEngineMock().__emit?.('edge:connected', { sourcePtr: 100, targetPtr: 200, edgeId: 'edge-ab' });
    });

    await waitFor(() => {
      const graph = JSON.parse(screen.getByTestId('graph-state').textContent ?? '{}');
      expect(graph.edges).toHaveLength(1);
      expect(graph.edges[0]).toMatchObject({ id: 'edge-ab', source: 'node-100', target: 'node-200' });
    });
  });

  it('skips duplicate edges for the same connection', async () => {
    const seededGraph = {
      ...baseGraph,
      edges: [{ id: 'edge-ab', source: 'node-100', target: 'node-200', animated: true }],
    };
    renderWithGraph(seededGraph);

    await act(async () => {
      getEngineMock().__emit?.('edge:connected', { sourcePtr: 100, targetPtr: 200, edgeId: 'edge-ab' });
    });

    await waitFor(() => {
      const graph = JSON.parse(screen.getByTestId('graph-state').textContent ?? '{}');
      expect(graph.edges).toHaveLength(1);
    });
  });

  it('removes edges when engine reports a disconnection', async () => {
    const seededGraph = {
      ...baseGraph,
      edges: [
        { id: 'edge-ab', source: 'node-100', target: 'node-200', animated: true },
        { id: 'edge-extra', source: 'node-100', target: 'ghost', animated: true },
      ],
    };
    renderWithGraph(seededGraph);

    await act(async () => {
      getEngineMock().__emit?.('edge:disconnected', { edgeId: 'edge-ab' });
    });

    await waitFor(() => {
      const graph = JSON.parse(screen.getByTestId('graph-state').textContent ?? '{}');
      expect(graph.edges).toHaveLength(1);
      expect(graph.edges[0].id).toBe('edge-extra');
    });
  });

  it('falls back to pointer matching when no edge id is provided', async () => {
    const seededGraph = {
      ...baseGraph,
      edges: [
        { id: 'node-100-node-200', source: 'node-100', target: 'node-200', animated: true },
        { id: 'other-edge', source: 'node-200', target: 'ghost', animated: true },
      ],
    };
    renderWithGraph(seededGraph);

    await act(async () => {
      getEngineMock().__emit?.('edge:disconnected', { sourcePtr: 200 });
    });

    await waitFor(() => {
      const graph = JSON.parse(screen.getByTestId('graph-state').textContent ?? '{}');
      expect(graph.edges).toHaveLength(1);
      expect(graph.edges[0].id).toBe('node-100-node-200');
    });
  });

  it('creates distinct edges for different slots between the same nodes', async () => {
    renderWithGraph(baseGraph);

    await act(async () => {
      getEngineMock().__emit?.('edge:connected', { sourcePtr: 100, targetPtr: 200, sourceSlot: 0, targetSlot: 0 });
      getEngineMock().__emit?.('edge:connected', { sourcePtr: 100, targetPtr: 200, sourceSlot: 1, targetSlot: 0 });
    });

    await waitFor(() => {
      const graph = JSON.parse(screen.getByTestId('graph-state').textContent ?? '{}');
      expect(graph.edges).toHaveLength(2);
      const ids = graph.edges.map((edge: any) => edge.id).sort();
      expect(ids).toEqual([
        'node-100:0->node-200:0',
        'node-100:1->node-200:0',
      ]);
    });
  });

  it('honors slot filters when disconnecting by pointer', async () => {
    const seededGraph = {
      ...baseGraph,
      edges: [
        { id: 'node-100:0->node-200:0', source: 'node-100', target: 'node-200', sourceSlot: 0, targetSlot: 0 },
        { id: 'node-100:1->node-200:0', source: 'node-100', target: 'node-200', sourceSlot: 1, targetSlot: 0 },
      ],
    } satisfies GraphSnapshot;

    renderWithGraph(seededGraph);

    await act(async () => {
      getEngineMock().__emit?.('edge:disconnected', { sourcePtr: 100, sourceSlot: 1 });
    });

    await waitFor(() => {
      const graph = JSON.parse(screen.getByTestId('graph-state').textContent ?? '{}');
      expect(graph.edges).toHaveLength(1);
      expect(graph.edges[0].sourceSlot).toBe(0);
    });
  });
});
