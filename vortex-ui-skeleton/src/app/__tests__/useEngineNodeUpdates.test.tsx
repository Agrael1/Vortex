import React, { useEffect } from 'react';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import { act, render, screen, waitFor } from '@testing-library/react';
import { RecoilRoot, useRecoilValue, useSetRecoilState } from 'recoil';
import { useEngineNodeUpdates } from '@state/hooks/useEngineNodeUpdates';
import { graphSnapshotAtom } from '@state/atoms/project';
import type { GraphSnapshot } from '@state/types';
import type { SetterOrUpdater } from 'recoil';

vi.mock('@/app/services/ipc/cefBridge', () => {
  const listeners: Array<(payload: { nodePtr: number; propIndex: number; value: unknown }) => void> = [];
  const getNodeProperties = vi.fn();
  return {
    engine: {
      on: (event: string, handler: (payload: { nodePtr: number; propIndex: number; value: unknown }) => void) => {
        if (event === 'node:update') {
          listeners.push(handler);
          return () => {
            const idx = listeners.indexOf(handler);
            if (idx >= 0) {
              listeners.splice(idx, 1);
            }
          };
        }
        return () => undefined;
      },
      getNodeProperties,
      __emitNodeUpdate: (payload: { nodePtr: number; propIndex: number; value: unknown }) => {
        listeners.forEach((listener) => listener(payload));
      },
      __reset: () => {
        listeners.length = 0;
      },
    },
  };
});

import { engine } from '@/app/services/ipc/cefBridge';

const mockGetNodeProperties = vi.mocked(engine.getNodeProperties);

type EngineMock = typeof engine & {
  __emitNodeUpdate?: (payload: { nodePtr: number; propIndex: number; value: unknown }) => void;
  __reset?: () => void;
};

const getMockedEngine = () => engine as EngineMock;

const emitNodeUpdate = (ptr: number, index: number, value: unknown) => {
  getMockedEngine().__emitNodeUpdate?.({ nodePtr: ptr, propIndex: index, value });
};

const resetNodeListeners = () => {
  getMockedEngine().__reset?.();
};

type GraphObserverProps = {
  onReady?: (setter: SetterOrUpdater<GraphSnapshot>) => void;
};

const GraphObserver = ({ onReady }: GraphObserverProps = {}) => {
  useEngineNodeUpdates();
  const setGraph = useSetRecoilState(graphSnapshotAtom);
  useEffect(() => {
    if (onReady) {
      onReady(setGraph);
    }
  }, [onReady, setGraph]);
  const graph = useRecoilValue(graphSnapshotAtom);
  return <pre data-testid="graph-state">{JSON.stringify(graph)}</pre>;
};

const defaultSchema = {
  properties: [
    { index: 1, name: 'position' },
    { index: 2, name: 'label' },
    { index: 3, name: 'status' },
  ],
};

const readGraph = () => {
  const raw = screen.getByTestId('graph-state').textContent ?? '{}';
  return JSON.parse(raw);
};

const PROPERTY_CACHE_TTL_MS = 60_000;

describe('useEngineNodeUpdates', () => {
  beforeEach(() => {
    mockGetNodeProperties.mockReset();
    mockGetNodeProperties.mockResolvedValue(defaultSchema);
    resetNodeListeners();
  });

  it('creates nodes and updates position payloads', async () => {
    render(
      <RecoilRoot>
        <GraphObserver />
      </RecoilRoot>,
    );

    emitNodeUpdate(100, 1, { x: 12, y: 24 });

    await waitFor(() => {
      const graph = readGraph();
      expect(graph.nodes).toHaveLength(1);
      expect(graph.nodes[0].position).toEqual({ x: 12, y: 24 });
      expect(graph.nodes[0].props.position).toEqual({ x: 12, y: 24 });
    });

    expect(mockGetNodeProperties).toHaveBeenCalledWith(100);
  });

  it('normalizes labels and stores generic props', async () => {
    render(
      <RecoilRoot>
        <GraphObserver />
      </RecoilRoot>,
    );

    emitNodeUpdate(42, 2, '');
    emitNodeUpdate(42, 3, 'working');

    await waitFor(() => {
      const graph = readGraph();
      expect(graph.nodes).toHaveLength(1);
      expect(graph.nodes[0].label).toBe('42');
      expect(graph.nodes[0].props.status).toBe('working');
    });

    expect(mockGetNodeProperties).toHaveBeenCalledTimes(1);
  });

  it('refreshes cached schemas once TTL elapses', async () => {
    let currentTime = Date.now();
    const dateSpy = vi.spyOn(Date, 'now').mockImplementation(() => currentTime);

    try {
      render(
        <RecoilRoot>
          <GraphObserver />
        </RecoilRoot>,
      );

      emitNodeUpdate(7, 3, 'working');

      await waitFor(() => {
        const graph = readGraph();
        expect(graph.nodes).toHaveLength(1);
        expect(graph.nodes[0].props.status).toBe('working');
      });

      expect(mockGetNodeProperties).toHaveBeenCalledTimes(1);

      currentTime += PROPERTY_CACHE_TTL_MS + 10;

      emitNodeUpdate(7, 3, 'done');

      await waitFor(() => {
        const graph = readGraph();
        expect(graph.nodes[0].props.status).toBe('done');
      });

      await waitFor(() => {
        expect(mockGetNodeProperties).toHaveBeenCalledTimes(2);
      });
    } finally {
      dateSpy.mockRestore();
    }
  });

  it('drops property cache entries once nodes are removed', async () => {
    let setGraph: SetterOrUpdater<GraphSnapshot> | null = null;

    render(
      <RecoilRoot>
        <GraphObserver onReady={(setter) => {
          setGraph = setter;
        }} />
      </RecoilRoot>,
    );

    await waitFor(() => {
      expect(setGraph).toBeTruthy();
    });

    emitNodeUpdate(15, 1, { x: 1, y: 2 });

    await waitFor(() => {
      const graph = readGraph();
      expect(graph.nodes).toHaveLength(1);
      expect(graph.nodes[0].position).toEqual({ x: 1, y: 2 });
    });

    expect(mockGetNodeProperties).toHaveBeenCalledTimes(1);

    act(() => {
      setGraph?.(() => ({ nodes: [], edges: [] }));
    });

    await waitFor(() => {
      const graph = readGraph();
      expect(graph.nodes).toHaveLength(0);
    });

    emitNodeUpdate(15, 1, { x: 5, y: 6 });

    await waitFor(() => {
      const graph = readGraph();
      expect(graph.nodes).toHaveLength(1);
      expect(graph.nodes[0].position).toEqual({ x: 5, y: 6 });
    });

    await waitFor(() => {
      expect(mockGetNodeProperties).toHaveBeenCalledTimes(2);
    });
  });
});
