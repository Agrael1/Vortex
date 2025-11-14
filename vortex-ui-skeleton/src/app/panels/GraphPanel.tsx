import { useCallback, useEffect, useRef, useState } from "react";
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
import { CustomNode, CustomNodeData } from "@/app/components/CustomNode";
import { engine, type LastProject } from "@/app/services/ipc/cefBridge";

export const DND_TYPE = "application/x-vortex-node-type";
const idFromPtr = (ptr: number) => String(ptr);

type RFNode = Node<CustomNodeData>;
type RFEdge = Edge;

const nodeTypes = {
  custom: CustomNode,
};

type Props = { onSelectPtr(ptr: number | null): void };

const GRAPH_STORAGE_PREFIX = 'vortex.editor.graph'

type StoredNode = {
  ptr: number
  type: string
  label: string
  position: { x: number; y: number }
}

type StoredEdge = {
  id: string
  source: string
  target: string
  animated?: boolean
}

type StoredGraph = {
  nodes: StoredNode[]
  edges: StoredEdge[]
}

const getGraphStorageKey = (projectPath: string | null | undefined) =>
  `${GRAPH_STORAGE_PREFIX}:${projectPath && projectPath.length ? projectPath : 'default'}`

const toRFNode = (node: StoredNode): RFNode => ({
  id: idFromPtr(node.ptr),
  type: 'custom',
  position: node.position ?? { x: 120, y: 120 },
  data: { label: node.label ?? node.type, ptr: node.ptr, type: node.type },
  sourcePosition: Position.Right,
  targetPosition: Position.Left,
})

const toRFEdge = (edge: StoredEdge): RFEdge => ({
  id: edge.id ?? `${edge.source}-${edge.target}`,
  source: edge.source,
  target: edge.target,
  animated: edge.animated ?? true,
})

const loadGraphState = (projectPath: string | null | undefined): { nodes: RFNode[]; edges: RFEdge[] } => {
  if (typeof window === 'undefined') return { nodes: [], edges: [] }

  try {
    const stored = window.localStorage.getItem(getGraphStorageKey(projectPath))
    if (!stored) return { nodes: [], edges: [] }
    const parsed = JSON.parse(stored) as StoredGraph

    const nodes = Array.isArray(parsed?.nodes) ? parsed.nodes.map(toRFNode) : []
    const edges = Array.isArray(parsed?.edges) ? parsed.edges.map(toRFEdge) : []
    return { nodes, edges }
  } catch (error) {
    console.warn('[GraphPanel] Failed to load graph state', error)
    return { nodes: [], edges: [] }
  }
}

const serializeGraph = (nodes: RFNode[], edges: RFEdge[]): StoredGraph => ({
  nodes: nodes.map((node) => ({
    ptr: node.data.ptr,
    type: node.data.type ?? node.data.label,
    label: node.data.label,
    position: node.position,
  })),
  edges: edges.map((edge) => ({
    id: edge.id,
    source: edge.source,
    target: edge.target,
    animated: edge.animated,
  })),
})

const computeNextSpawn = (nodeList: RFNode[]) => {
  if (!nodeList.length) {
    return { x: 120, y: 120 }
  }
  const last = nodeList[nodeList.length - 1]
  return { x: last.position.x + 40, y: last.position.y + 40 }
}

function GraphInner({ onSelectPtr }: Props) {
  const [projectKey, setProjectKey] = useState<string>(() => engine.getLastProject()?.path ?? 'default')
  const initialGraph = useRef(loadGraphState(projectKey))
  const [nodes, setNodes, onNodesChange] = useNodesState<RFNode>(initialGraph.current.nodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState<RFEdge>(initialGraph.current.edges);
  const rf = useReactFlow();
  const [spawn, setSpawn] = useState(() => computeNextSpawn(initialGraph.current.nodes));
  const previousNodeCountRef = useRef<number>(initialGraph.current.nodes.length);

  const addNode = useCallback(
    async (type: string, pos?: { x: number; y: number }): Promise<number> => {
      const ptr = await Vortex.createNode(type);
      const id = idFromPtr(ptr);
      const position = pos ?? spawn;

      const n: RFNode = {
        id,
        type: 'custom',
        position,
        data: { label: type, ptr, type },
        sourcePosition: Position.Right,
        targetPosition: Position.Left,
      };

      setNodes((nds) => nds.concat(n));
      onSelectPtr(ptr);
      return ptr;
    },
    [onSelectPtr, setNodes, spawn]
  );

  // access from AppShell (and console) - now returns ptr
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

  useEffect(() => {
    const off = engine.on('lastProject:updated', (payload) => {
      const detail = payload as LastProject | null
      const nextKey = detail?.path ?? 'default'
      setProjectKey((current) => (current === nextKey ? current : nextKey))
    })

    return () => {
      off?.()
    }
  }, [])

  useEffect(() => {
    const snapshot = loadGraphState(projectKey)
    setNodes(snapshot.nodes)
    setEdges(snapshot.edges)
    previousNodeCountRef.current = snapshot.nodes.length
    setSpawn(computeNextSpawn(snapshot.nodes))
    onSelectPtr(null)
  }, [onSelectPtr, projectKey, setEdges, setNodes])

  useEffect(() => {
    if (typeof window === 'undefined') return
    try {
      const payload = serializeGraph(nodes, edges)
      window.localStorage.setItem(getGraphStorageKey(projectKey), JSON.stringify(payload))
    } catch (error) {
      console.warn('[GraphPanel] Failed to persist graph state', error)
    }
  }, [edges, nodes, projectKey])

  useEffect(() => {
    const currentCount = nodes.length
    if (currentCount === 0) {
      setSpawn({ x: 120, y: 120 })
    } else if (currentCount > previousNodeCountRef.current) {
      const last = nodes[currentCount - 1]
      setSpawn({ x: last.position.x + 40, y: last.position.y + 40 })
    } else if (currentCount < previousNodeCountRef.current) {
      setSpawn(computeNextSpawn(nodes))
    }

    previousNodeCountRef.current = currentCount
  }, [nodes])

  return (
    <div
      style={{ width: "100%", height: "100%" }}
      onDrop={onDrop}
      onDragOver={onDragOver}
    >
      <ReactFlow
        nodes={nodes}
        edges={edges}
        nodeTypes={nodeTypes}
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
