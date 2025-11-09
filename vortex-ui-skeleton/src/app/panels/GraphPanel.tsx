import { useCallback, useEffect, useState } from "react";
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
} from "@xyflow/react";
import { Vortex } from "@/bridge/vortex";

export const DND_TYPE = "application/x-vortex-node-type";
const idFromPtr = (ptr: number) => String(ptr);

// данные ноды
type RFNodeData = { label: string; ptr: number };
// типы ноды/ребра для state
type RFNode = Node<RFNodeData>;
type RFEdge = Edge;

type Props = { onSelectPtr(ptr: number | null): void };

function GraphInner({ onSelectPtr }: Props) {
  // <<< ВАЖНО: передаём тип НОДЫ (RFNode), не массив и не RFNodeData
  const [nodes, setNodes, onNodesChange] = useNodesState<RFNode>([]);
  const [edges, setEdges, onEdgesChange] = useEdgesState<RFEdge>([]);
  const rf = useReactFlow();

  const [spawn, setSpawn] = useState({ x: 120, y: 120 });

  const addNode = useCallback(
    async (type: string, pos?: { x: number; y: number }) => {
      const ptr = await Vortex.createNode(type);
      const id = idFromPtr(ptr);
      const position = pos ?? spawn;

      const n: RFNode = {
        id,
        position,
        data: { label: type, ptr },
        sourcePosition: Position.Right,
        targetPosition: Position.Left,
        style: {
          padding: 8,
          borderRadius: 6,
          background: "#121212",
          color: "#ddd",
          border: "1px solid #2a2a2a",
        },
      };

      setNodes((nds) => nds.concat(n));
      setSpawn((s) => ({ x: s.x + 40, y: s.y + 40 }));
      onSelectPtr(ptr);
    },
    [onSelectPtr, setNodes, spawn]
  );

  useEffect(() => {
    (window as any).__GraphPanelAddNode = (
      type: string,
      x?: number,
      y?: number
    ) => addNode(type, x != null && y != null ? { x, y } : undefined);
  }, [addNode]);

  const onConnect = useCallback<OnConnect>(
    async (conn: Connection) => {
      setEdges((eds) => addEdge({ ...conn, animated: true } as RFEdge, eds));
      const srcPtr = Number(conn.source);
      const dstPtr = Number(conn.target);
      await Vortex.connect(srcPtr, 0, dstPtr, 0);
    },
    [setEdges]
  );

  const onSelectionChange = useCallback(
    (params: { nodes: RFNode[] }) => {
      const first = params.nodes[0];
      onSelectPtr(first ? first.data.ptr : null);
    },
    [onSelectPtr]
  );

  const onDrop = useCallback(
    (ev: React.DragEvent) => {
      ev.preventDefault();
      const type = ev.dataTransfer.getData(DND_TYPE);
      const pos = rf.screenToFlowPosition({ x: ev.clientX, y: ev.clientY });
      if (type) addNode(type, pos);
    },
    [rf, addNode]
  );

  const onDragOver = useCallback((ev: React.DragEvent) => {
    ev.preventDefault();
    ev.dataTransfer.dropEffect = "move";
  }, []);

  return (
    <div
      style={{ width: "100%", height: "100%" }}
      onDrop={onDrop}
      onDragOver={onDragOver}
    >
      <ReactFlow
        nodes={nodes}
        edges={edges}
        onConnect={onConnect}
        onNodesChange={onNodesChange}
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

export function GraphPanel(props: Props) {
  return (
    <ReactFlowProvider>
      <GraphInner {...props} />
    </ReactFlowProvider>
  );
}
