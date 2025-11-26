import { useCallback, useEffect, useRef } from 'react';
import {
  Background,
  Controls,
  MiniMap,
  ReactFlow,
  addEdge,
  useEdgesState,
  useNodesState,
  Connection,
  Edge,
  Node,
  OnConnect,
  ReactFlowProvider,
  useReactFlow,
  Position,
} from '@xyflow/react';
import type { DragEvent } from 'react';
import type { NodeChange } from '@xyflow/react';
import { useRecoilValue, useSetRecoilState } from 'recoil';
import { CustomNode, CustomNodeData } from '@/app/components/CustomNode';
import { graphSnapshotAtom } from '@state/atoms/project';
import { selectedNodePtrAtom } from '@state/atoms/editor';
import { useGraphCommands } from '@state/hooks/useGraphCommands';
import type { GraphEdgeSnapshot, GraphNodeSnapshot, GraphSnapshot } from '@state/types';

export const DND_TYPE = 'application/x-vortex-node-type';
const idFromPtr = (ptr: number) => String(ptr);

type RFNode = Node<CustomNodeData>;
type RFEdge = Edge;

declare global {
  interface Window {
    __VortexDebugSelection?: boolean;
    __VortexSkipSelectionSync?: boolean;
  }
}

const nodeTypes = {
  custom: CustomNode,
};

const toRFNode = (node: GraphNodeSnapshot): RFNode => ({
  id: node.id ?? idFromPtr(node.ptr ?? 0),
  type: 'custom',
  position: node.position ?? { x: 120, y: 120 },
  data: {
    label: node.label ?? node.type,
    ptr: node.ptr ?? Number(node.id),
    type: node.type,
    props: node.props,
  },
  sourcePosition: Position.Right,
  targetPosition: Position.Left,
});

const toRFEdge = (edge: GraphEdgeSnapshot): RFEdge => ({
  id: edge.id ?? `${edge.source}-${edge.target}`,
  source: edge.source,
  target: edge.target,
  animated: edge.animated ?? true,
  data: {
    sourceSlot: edge.sourceSlot,
    targetSlot: edge.targetSlot,
  },
});

const toSnapshotNode = (node: RFNode): GraphNodeSnapshot => ({
  id: node.id,
  type: node.data.type ?? node.data.label ?? 'Node',
  label: node.data.label,
  position: node.position ?? { x: 120, y: 120 },
  ptr: Number.isFinite(node.data.ptr) ? Number(node.data.ptr) : undefined,
  props: node.data.props && typeof node.data.props === 'object' ? (node.data.props as Record<string, any>) : undefined,
});

const toSnapshotEdge = (edge: RFEdge): GraphEdgeSnapshot => ({
  id: edge.id,
  source: edge.source,
  target: edge.target,
  animated: edge.animated,
  sourceSlot: typeof edge.data?.sourceSlot === 'number' ? edge.data.sourceSlot : undefined,
  targetSlot: typeof edge.data?.targetSlot === 'number' ? edge.data.targetSlot : undefined,
});

const snapshotToReactFlow = (snapshot: GraphSnapshot, selectedPtr?: number | null): { nodes: RFNode[]; edges: RFEdge[] } => {
  const selectedId = selectedPtr != null ? String(selectedPtr) : null;
  const nodes =
    snapshot.nodes?.map((node) => {
      const rfNode = toRFNode(node);
      const isSelected = selectedPtr != null && (node.ptr === selectedPtr || node.id === selectedId);
      if (isSelected) {
        rfNode.selected = true;
      }
      return rfNode;
    }) ?? [];

  return {
    nodes,
    edges: snapshot.edges?.map(toRFEdge) ?? [],
  };
};

const reactFlowToSnapshot = (nodes: RFNode[], edges: RFEdge[]): GraphSnapshot => ({
  nodes: nodes.map(toSnapshotNode),
  edges: edges.map(toSnapshotEdge),
});

function GraphInner() {
  const graphSnapshot = useRecoilValue(graphSnapshotAtom);
  const setGraphSnapshot = useSetRecoilState(graphSnapshotAtom);
  const setSelectedPtr = useSetRecoilState(selectedNodePtrAtom);
  const selectedPtr = useRecoilValue(selectedNodePtrAtom);
  const { createNode, removeNode, removeEdges, connectNodes, updateNodePosition } = useGraphCommands();
  const initialGraph = useRef(snapshotToReactFlow(graphSnapshot));
  const [nodes, setNodes, onNodesChange] = useNodesState<RFNode>(initialGraph.current.nodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState<RFEdge>(initialGraph.current.edges);
  const rf = useReactFlow();
  const lastHydratedRef = useRef<string>(JSON.stringify(graphSnapshot));
  const selectedEdgesRef = useRef<RFEdge[]>([]);
  const previousSelectionRef = useRef<number | null>(null);
  const debugSelection = typeof window !== 'undefined' ? window.__VortexDebugSelection !== false : false;
  const skipSelectionSync = typeof window !== 'undefined' ? window.__VortexSkipSelectionSync === true : false;

  const logSelection = useCallback(
    (...args: unknown[]) => {
      if (debugSelection) {
        console.debug('[GraphPanel]', ...args);
      }
    },
    [debugSelection],
  );

  useEffect(() => {
    if (debugSelection) {
      console.debug('[GraphPanel] selection debugging enabled. Toggle window.__VortexDebugSelection = false to disable.');
    }
  }, [debugSelection]);

  useEffect(() => {
    if (debugSelection) {
      console.debug('[GraphPanel] window.__VortexSkipSelectionSync =', skipSelectionSync);
    }
  }, [debugSelection, skipSelectionSync]);

  useEffect(() => {
    if (debugSelection) {
      console.debug('[GraphPanel] Recoil selectedPtr observed ->', selectedPtr);
    }
  }, [debugSelection, selectedPtr]);
  useEffect(() => {
    const handler = (type: string, x?: number, y?: number) => createNode(type, x != null && y != null ? { x, y } : undefined);
    (window as any).__GraphPanelAddNode = handler;
    return () => {
      if ((window as any).__GraphPanelAddNode === handler) {
        delete (window as any).__GraphPanelAddNode;
      }
    };
  }, [createNode]);

  const onConnect = useCallback<OnConnect>(
    async (conn: Connection) => {
      if (!conn.source || !conn.target) {
        return;
      }

      setEdges((eds) => addEdge({ ...conn, animated: true } as RFEdge, eds));
      try {
        await connectNodes({ source: conn.source, target: conn.target });
      } catch (error) {
        console.error('[GraphPanel] Failed to connect nodes', error);
      }
    },
    [connectNodes, setEdges],
  );

  const onSelectionChange = useCallback(
    (params: { nodes: RFNode[]; edges: RFEdge[] }) => {
      const first = params.nodes[0];
      const rawPtr = first?.data?.ptr;
      const nextPtr = typeof rawPtr === 'number' && Number.isFinite(rawPtr) ? rawPtr : null;
      logSelection('selection event', {
        previous: previousSelectionRef.current,
        rawPtr,
        normalizedPtr: nextPtr,
        nodeId: first?.id ?? null,
        nodes: params.nodes.map((node) => ({ id: node.id, ptr: node.data.ptr })),
      });
      selectedEdgesRef.current = params.edges ?? [];

      if (skipSelectionSync) {
        logSelection('skipSelectionSync flag is true, aborting selection sync');
        return;
      }

      if (first && nextPtr == null) {
        logSelection('selection target missing numeric ptr, ignoring until engine provides one', {
          nodeId: first.id,
          rawPtr,
        });
        previousSelectionRef.current = null;
        return;
      }

      setSelectedPtr((current) => {
        if (current === (nextPtr ?? null)) {
          logSelection('selection already synced, skipping update');
          previousSelectionRef.current = current;
          return current;
        }
        const normalized = nextPtr ?? null;
        logSelection('updating selectedPtr', { previous: current, next: normalized, stack: new Error().stack });
        previousSelectionRef.current = normalized;
        return normalized;
      });
    },
    [logSelection, setSelectedPtr, skipSelectionSync],
  );

  const onDrop = useCallback(
    (ev: DragEvent<HTMLDivElement>) => {
      ev.preventDefault();
      const type = ev.dataTransfer.getData(DND_TYPE);
      const pos = rf.screenToFlowPosition({ x: ev.clientX, y: ev.clientY });
      if (type) {
        createNode(type, pos).catch((error) => {
          console.error('[GraphPanel] Failed to create node from drop', error);
        });
      }
    },
    [rf, createNode],
  );

  const onDragOver = useCallback((ev: DragEvent<HTMLDivElement>) => {
    ev.preventDefault();
    ev.dataTransfer.dropEffect = 'move';
  }, []);

  useEffect(() => {
    const isInteractiveTarget = (target: EventTarget | null): boolean => {
      if (!(target instanceof HTMLElement)) return false;
      const tag = target.tagName?.toLowerCase();
      if (!tag) return false;
      if (target.isContentEditable) return true;
      return tag === 'input' || tag === 'textarea' || tag === 'select' || tag === 'button';
    };

    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key !== 'Delete' && event.key !== 'Backspace') return;
      if (isInteractiveTarget(event.target)) return;

      if (selectedPtr) {
        event.preventDefault();
        removeNode(selectedPtr).catch((error) => {
          console.error('[GraphPanel] Failed to remove node', error);
        });
        return;
      }

      const selectedEdges = selectedEdgesRef.current;
      if (selectedEdges.length) {
        event.preventDefault();
        removeEdges(selectedEdges.map(toSnapshotEdge))
          .catch((error) => {
            console.error('[GraphPanel] Failed to remove edges', error);
          })
          .finally(() => {
            selectedEdgesRef.current = [];
          });
      }
    };

    document.addEventListener('keydown', handleKeyDown);
    return () => {
      document.removeEventListener('keydown', handleKeyDown);
    };
  }, [removeEdges, removeNode, selectedPtr]);

  useEffect(() => {
    const serialized = JSON.stringify(graphSnapshot);
    if (serialized === lastHydratedRef.current) {
      return;
    }

    const nextGraph = snapshotToReactFlow(graphSnapshot, selectedPtr);
    setNodes(nextGraph.nodes);
    setEdges(nextGraph.edges);

    const selectionStillExists =
      selectedPtr != null && graphSnapshot.nodes?.some((node) => node.ptr === selectedPtr || node.id === String(selectedPtr));

    if (!selectionStillExists) {
      setSelectedPtr(null);
    }

    lastHydratedRef.current = serialized;
  }, [graphSnapshot, selectedPtr, setEdges, setNodes, setSelectedPtr]);

  useEffect(() => {
    const snapshot = reactFlowToSnapshot(nodes, edges);
    const serialized = JSON.stringify(snapshot);
    if (serialized === lastHydratedRef.current) {
      return;
    }

    lastHydratedRef.current = serialized;
    setGraphSnapshot(snapshot);
  }, [edges, nodes, setGraphSnapshot]);

  const handleNodesChange = useCallback(
    (changes: NodeChange<RFNode>[]) => {
      onNodesChange(changes);

      const updates = changes.flatMap((change) => {
        if (change.type !== 'position') return [];
        if (change.dragging) return [];

        const position = change.position ?? change.positionAbsolute;
        if (!position) return [];

        return [{ id: change.id, position }];
      });

      updates.forEach(({ id, position }) => {
        updateNodePosition(id, position).catch((error) => {
          console.error('[GraphPanel] Failed to sync node position', { id, error });
        });
      });
    },
    [onNodesChange, updateNodePosition],
  );

  return (
    <div style={{ width: '100%', height: '100%' }} onDrop={onDrop} onDragOver={onDragOver}>
      <ReactFlow
        nodes={nodes}
        edges={edges}
        nodeTypes={nodeTypes}
        onConnect={onConnect}
        onNodesChange={handleNodesChange}
        onEdgesChange={onEdgesChange}
        onSelectionChange={onSelectionChange}
        fitView
      >
        <MiniMap />
        <Controls />
        <Background />
      </ReactFlow>
    </div>
  );
}

export function GraphPanel() {
  return (
    <ReactFlowProvider>
      <GraphInner />
    </ReactFlowProvider>
  );
}
