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
import { Vortex } from '@/bridge/vortex';
import { CustomNode, CustomNodeData } from '@/app/components/CustomNode';
import { graphSnapshotAtom } from '@state/atoms/project';
import { selectedNodePtrAtom } from '@state/atoms/editor';
import { useGraphCommands } from '@state/hooks/useGraphCommands';
import type { GraphEdgeSnapshot, GraphNodeSnapshot, GraphSnapshot } from '@state/types';

export const DND_TYPE = 'application/x-vortex-node-type';
const idFromPtr = (ptr: number) => String(ptr);

type RFNode = Node<CustomNodeData>;
type RFEdge = Edge;

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
  const { createNode, removeNode, removeEdges } = useGraphCommands();
  const initialGraph = useRef(snapshotToReactFlow(graphSnapshot));
  const [nodes, setNodes, onNodesChange] = useNodesState<RFNode>(initialGraph.current.nodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState<RFEdge>(initialGraph.current.edges);
  const rf = useReactFlow();
  const lastHydratedRef = useRef<string>(JSON.stringify(graphSnapshot));
  const selectedEdgesRef = useRef<RFEdge[]>([]);
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
      setEdges((eds) => addEdge({ ...conn, animated: true } as RFEdge, eds));
      const srcPtr = Number(conn.source);
      const dstPtr = Number(conn.target);
      await Vortex.connect(srcPtr, 0, dstPtr, 0);
    },
    [setEdges],
  );

  const onSelectionChange = useCallback(
    (params: { nodes: RFNode[]; edges: RFEdge[] }) => {
      const first = params.nodes[0];
      setSelectedPtr(first ? first.data.ptr : null);
      selectedEdgesRef.current = params.edges ?? [];
    },
    [setSelectedPtr],
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

        const node = nodes.find((n) => n.id === change.id);
        const ptr = node?.data.ptr;
        if (!Number.isFinite(ptr)) return [];

        return [{ ptr: Number(ptr), position }];
      });

      updates.forEach(({ ptr, position }) => {
        const payload = JSON.stringify(position);
        Vortex.setNodeProperty(ptr, 'position', payload).catch((error) => {
          console.error('[GraphPanel] Failed to sync node position', { ptr, error });
        });
      });
    },
    [nodes, onNodesChange],
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
