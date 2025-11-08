// src/app/panels/GraphPanel.tsx
import { useCallback } from 'react';
import {
  ReactFlow,
  Background,
  Controls,
  MiniMap,
  addEdge,
  useEdgesState,
  useNodesState,
  type Connection,
  type Edge,
  type Node,
} from '@xyflow/react';
import '@xyflow/react/dist/style.css';
import { engine } from '@/bridge/engine';

type Ptr = number;

export function GraphPanel({ onSelectPtr }: { onSelectPtr: (ptr: Ptr | null) => void }) {
  const [nodes, setNodes, onNodesChange] = useNodesState<Node>([]);
  const [edges, setEdges, onEdgesChange] = useEdgesState<Edge>([]);

  const idFromPtr = (ptr: Ptr) => String(ptr);

  // окно (AppShell) вызывает это через window-хук
  const addNodeFromType = useCallback(
    (typeName: string, ptr: Ptr) => {
      const id = idFromPtr(ptr);
      const n: Node = {
        id,
        position: { x: 120 + Math.random() * 240, y: 120 + Math.random() * 120 },
        data: { label: typeName, ptr },
        type: 'default',
      };
      setNodes((nds) => nds.concat(n));
    },
    [setNodes]
  );

  const onConnect = useCallback(
    async (conn: Connection) => {
      if (!conn.source || !conn.target) return;
      const left = nodes.find((n) => n.id === conn.source);
      const right = nodes.find((n) => n.id === conn.target);
      if (!left || !right) return;

      const lPtr = Number((left.data as any)?.ptr);
      const rPtr = Number((right.data as any)?.ptr);
      const lOut = Number(conn.sourceHandle ?? 0);
      const rIn = Number(conn.targetHandle ?? 0);

      const ok = await engine.connect(lPtr, lOut, rPtr, rIn);
      if (ok) setEdges((eds) => addEdge(conn, eds));
      else console.warn('Connect failed by engine');
    },
    [nodes, setEdges]
  );

  const onNodesDelete = useCallback((deleted: Node[]) => {
    deleted.forEach((n) => {
      const ptr = Number((n.data as any)?.ptr);
      if (Number.isFinite(ptr)) engine.removeNode(ptr);
    });
  }, []);

  const onSelectionChange = useCallback(
    ({ nodes: sel }: { nodes: Node[] }) => {
      if (sel.length > 0) {
        const ptr = Number(sel[0].data?.ptr);
        onSelectPtr(Number.isFinite(ptr) ? ptr : null);
      } else {
        onSelectPtr(null);
      }
    },
    [onSelectPtr]
  );

  // прокинем хук в window
  (window as any).__GraphPanelAddNode = addNodeFromType;

  return (
    <div style={{ width: '100%', height: '100%', minHeight: 0 }}>
      <ReactFlow
        nodes={nodes}
        edges={edges}
        onNodesChange={onNodesChange}
        onEdgesChange={onEdgesChange}
        onConnect={onConnect}
        onNodesDelete={onNodesDelete}
        onSelectionChange={onSelectionChange}
        fitView
      >
        <Background />
        <MiniMap />
        <Controls />
      </ReactFlow>
    </div>
  );
}
