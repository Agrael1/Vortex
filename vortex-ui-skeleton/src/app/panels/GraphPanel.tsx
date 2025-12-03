import { useCallback, useEffect, useRef, useState } from 'react';
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
import type { CSSProperties, DragEvent, MouseEvent as ReactMouseEvent } from 'react';
import type { NodeChange } from '@xyflow/react';
import { useRecoilValue, useSetRecoilState } from 'recoil';
import { CustomNode, CustomNodeData } from '@/app/components/CustomNode';
import { graphSnapshotAtom } from '@state/atoms/project';
import { nodePtrByIdAtom, nodePtrByUidAtom, selectedNodeIdentityAtom, selectedNodePtrAtom } from '@state/atoms/editor';
import { useGraphCommands } from '@state/hooks/useGraphCommands';
import type { GraphEdgeSnapshot, GraphNodeSnapshot, GraphSnapshot } from '@state/types';

export const DND_TYPE = 'application/x-vortex-node-type';
const idFromPtr = (ptr: number) => String(ptr);

type RFNode = Node<CustomNodeData>;
type RFEdge = Edge;

type EdgeBubbleState = {
  edge: RFEdge;
  position: { x: number; y: number };
};

type NodeBubbleState = {
  node: RFNode;
  position: { x: number; y: number };
};

type BubblePosition = { x: number; y: number };

const ACTION_BUBBLE_STYLE: CSSProperties = {
  position: 'absolute',
  transform: 'translate(-50%, -100%) translateY(-8px)',
  background: 'rgba(12, 12, 20, 0.95)',
  color: '#f5f5f5',
  borderRadius: 8,
  border: '1px solid rgba(255, 255, 255, 0.18)',
  boxShadow: '0 10px 24px rgba(0, 0, 0, 0.35)',
  padding: '6px 12px',
  display: 'flex',
  alignItems: 'center',
  gap: 10,
  pointerEvents: 'auto',
  zIndex: 10,
  minWidth: 120,
};

const ACTION_BUBBLE_LABEL_STYLE: CSSProperties = {
  fontSize: 12,
  letterSpacing: '0.08em',
  textTransform: 'uppercase',
  opacity: 0.9,
};

const ACTION_BUBBLE_BUTTON_STYLE: CSSProperties = {
  width: 28,
  height: 28,
  borderRadius: '50%',
  border: '1px solid rgba(255, 255, 255, 0.25)',
  background: 'transparent',
  color: '#f5f5f5',
  fontSize: 14,
  lineHeight: 1,
  display: 'inline-flex',
  alignItems: 'center',
  justifyContent: 'center',
  cursor: 'pointer',
};

const DeleteBubble = ({ position, onConfirm }: { position: BubblePosition; onConfirm: () => void }) => (
  <div
    style={{
      ...ACTION_BUBBLE_STYLE,
      left: position.x,
      top: position.y,
    }}
  >
    <span style={ACTION_BUBBLE_LABEL_STYLE}>Deleted</span>
    <button type="button" onClick={onConfirm} style={ACTION_BUBBLE_BUTTON_STYLE} aria-label="Delete">
      ✕
    </button>
  </div>
);

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
    uid: node.uid ?? null,
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
  uid: typeof node.data?.uid === 'string' && node.data.uid.length ? node.data.uid : null,
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

const snapshotToReactFlow = (
  snapshot: GraphSnapshot,
  selection?: { ptr?: number | null; uid?: string | null; id?: string | null },
): { nodes: RFNode[]; edges: RFEdge[] } => {
  const selectedPtr = selection?.ptr ?? null;
  const selectedUid = selection?.uid ?? null;
  const selectedId = selection?.id ?? (selectedPtr != null ? String(selectedPtr) : null);
  const nodes =
    snapshot.nodes?.map((node) => {
      const rfNode = toRFNode(node);
      const isSelected = Boolean(
        (selectedPtr != null && (node.ptr === selectedPtr || node.id === String(selectedPtr))) ||
          (selectedUid && node.uid && node.uid === selectedUid) ||
          (selectedId && node.id === selectedId),
      );
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
  const setSelectedIdentity = useSetRecoilState(selectedNodeIdentityAtom);
  const selectedIdentity = useRecoilValue(selectedNodeIdentityAtom);
  const setNodePtrById = useSetRecoilState(nodePtrByIdAtom);
  const setNodePtrByUid = useSetRecoilState(nodePtrByUidAtom);
  const ptrById = useRecoilValue(nodePtrByIdAtom);
  const ptrByUid = useRecoilValue(nodePtrByUidAtom);
  const { createNode, removeNode, removeEdges, connectNodes, updateNodePosition } = useGraphCommands();
  const initialGraph = useRef(snapshotToReactFlow(graphSnapshot));
  const [nodes, setNodes, onNodesChange] = useNodesState<RFNode>(initialGraph.current.nodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState<RFEdge>(initialGraph.current.edges);
  const rf = useReactFlow();
  const lastHydratedRef = useRef<string>(JSON.stringify(graphSnapshot));
  const selectedEdgesRef = useRef<RFEdge[]>([]);
  const previousSelectionRef = useRef<number | null>(null);
  const graphSnapshotRef = useRef<GraphSnapshot>(graphSnapshot);
  const debugSelection = typeof window !== 'undefined' ? window.__VortexDebugSelection !== false : false;
  const skipSelectionSync = typeof window !== 'undefined' ? window.__VortexSkipSelectionSync === true : false;
  const containerRef = useRef<HTMLDivElement>(null);
  const [edgeBubble, setEdgeBubble] = useState<EdgeBubbleState | null>(null);
  const [nodeBubble, setNodeBubble] = useState<NodeBubbleState | null>(null);

  graphSnapshotRef.current = graphSnapshot;

  const nodeExistsInSnapshot = useCallback(
    (identity: { ptr?: number | null; id?: string | null; uid?: string | null }) => {
      const snapshot = graphSnapshotRef.current;
      const nodes = snapshot.nodes ?? [];
      return nodes.some((node) => {
        if (!node) return false;
        if (identity.ptr != null && (node.ptr === identity.ptr || node.id === String(identity.ptr))) {
          return true;
        }
        if (identity.uid && node.uid === identity.uid) {
          return true;
        }
        if (identity.id && node.id === identity.id) {
          return true;
        }
        return false;
      });
    },
    [],
  );

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
      const nextUid = typeof first?.data?.uid === 'string' && first.data.uid.trim().length ? first.data.uid.trim() : null;
      logSelection('selection event', {
        previous: previousSelectionRef.current,
        rawPtr,
        normalizedPtr: nextPtr,
        nodeId: first?.id ?? null,
        nodes: params.nodes.map((node) => ({ id: node.id, ptr: node.data.ptr, uid: node.data.uid })),
      });
      selectedEdgesRef.current = params.edges ?? [];
      setEdgeBubble((current) => {
        if (!current) {
          return current;
        }
        const stillSelected = (params.edges ?? []).some((edge) => edge.id === current.edge.id);
        return stillSelected ? current : null;
      });
      setNodeBubble(null);

      if (skipSelectionSync) {
        logSelection('skipSelectionSync flag is true, aborting selection sync');
        return;
      }

      const snapshotHasNode = nodeExistsInSnapshot({ ptr: nextPtr, id: first?.id ?? null, uid: nextUid });
      if (!snapshotHasNode) {
        logSelection('ignoring selection for node missing from current snapshot', {
          ptr: nextPtr,
          id: first?.id ?? null,
          uid: nextUid,
        });
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

      setSelectedIdentity((prev) => {
        if (!first || nextPtr == null) {
          if (prev.id === null && prev.uid === null && prev.ptr === null) {
            return prev;
          }
          return { id: null, uid: null, ptr: null };
        }
        const nextId = first.id ?? (nextPtr != null ? String(nextPtr) : null);
        if (prev.id === nextId && prev.uid === nextUid && prev.ptr === nextPtr) {
          return prev;
        }
        return { id: nextId ?? null, uid: nextUid, ptr: nextPtr };
      });
    },
    [logSelection, nodeExistsInSnapshot, setSelectedIdentity, setSelectedPtr, skipSelectionSync],
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

  const handleEdgeClick = useCallback(
    (event: ReactMouseEvent<Element, MouseEvent>, edge: RFEdge) => {
      event.stopPropagation();
      const rect = containerRef.current?.getBoundingClientRect();
      const x = event.clientX - (rect?.left ?? 0);
      const y = event.clientY - (rect?.top ?? 0);
      setNodeBubble(null);
      setEdgeBubble({ edge, position: { x, y } });
    },
    [],
  );

  const closeActionBubbles = useCallback(() => {
    setEdgeBubble(null);
    setNodeBubble(null);
  }, []);

  const handleEdgeDelete = useCallback(() => {
    setNodeBubble(null);
    setEdgeBubble((current) => {
      if (!current) {
        return current;
      }
      removeEdges([toSnapshotEdge(current.edge)])
        .catch((error) => {
          console.error('[GraphPanel] Failed to remove edge from bubble', error);
        })
        .finally(() => {
          selectedEdgesRef.current = [];
        });
      return null;
    });
  }, [removeEdges]);

  const handleNodeDoubleClick = useCallback(
    (event: ReactMouseEvent<Element, MouseEvent>, node: RFNode) => {
      event.stopPropagation();
      const rect = containerRef.current?.getBoundingClientRect();
      const x = event.clientX - (rect?.left ?? 0);
      const y = event.clientY - (rect?.top ?? 0);
      setEdgeBubble(null);
      setNodeBubble({ node, position: { x, y } });
    },
    [],
  );

  const handleNodeDelete = useCallback(() => {
    setNodeBubble((current) => {
      if (!current) {
        return current;
      }
      const ptr = typeof current.node.data?.ptr === 'number' && Number.isFinite(current.node.data.ptr)
        ? current.node.data.ptr
        : Number.isFinite(Number(current.node.id))
          ? Number(current.node.id)
          : null;
      if (!ptr) {
        console.warn('[GraphPanel] Unable to resolve node pointer for deletion bubble');
        return null;
      }
      removeNode(ptr).catch((error) => {
        console.error('[GraphPanel] Failed to remove node from bubble', error);
      });
      return null;
    });
  }, [removeNode]);

  useEffect(() => {
    const isInteractiveTarget = (target: EventTarget | null): boolean => {
      if (!(target instanceof HTMLElement)) return false;
      const tag = target.tagName?.toLowerCase();
      if (!tag) return false;
      if (target.isContentEditable) return true;
      return tag === 'input' || tag === 'textarea' || tag === 'select' || tag === 'button';
    };

    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape') {
        if (!isInteractiveTarget(event.target)) {
          closeActionBubbles();
        }
        return;
      }

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
  }, [closeActionBubbles, removeEdges, removeNode, selectedPtr]);

  useEffect(() => {
    const serialized = JSON.stringify(graphSnapshot);
    if (serialized === lastHydratedRef.current) {
      return;
    }

    const nextGraph = snapshotToReactFlow(graphSnapshot, {
      ptr: selectedPtr,
      uid: selectedIdentity.uid,
      id: selectedIdentity.id,
    });
    setNodes(nextGraph.nodes);
    setEdges(nextGraph.edges);

    const snapshotNodes = graphSnapshot.nodes ?? [];
    let canonicalNode: GraphNodeSnapshot | null = null;
    if (selectedPtr != null) {
      canonicalNode =
        snapshotNodes.find((node) => node.ptr === selectedPtr || (!!node.id && node.id === String(selectedPtr))) ?? null;
    }
    if (!canonicalNode && selectedIdentity.uid) {
      canonicalNode = snapshotNodes.find((node) => node.uid && node.uid === selectedIdentity.uid) ?? null;
    }
    if (!canonicalNode && selectedIdentity.id) {
      canonicalNode = snapshotNodes.find((node) => node.id === selectedIdentity.id) ?? null;
    }

    if (!canonicalNode) {
      if (selectedPtr != null || selectedIdentity.id || selectedIdentity.uid) {
        setSelectedPtr(null);
        setSelectedIdentity({ id: null, uid: null, ptr: null });
      }
    } else {
      const ptrFromNode = typeof canonicalNode.ptr === 'number' && Number.isFinite(canonicalNode.ptr)
        ? canonicalNode.ptr
        : null;
      const numericId = Number(canonicalNode.id);
      const canonicalPtr = ptrFromNode ?? (Number.isFinite(numericId) ? numericId : null);

      if (canonicalPtr != null) {
        setSelectedPtr((current) => (current === canonicalPtr ? current : canonicalPtr));
      }

      setSelectedIdentity((prev) => {
        const nextId = canonicalNode.id ?? prev.id ?? null;
        const nextUid = canonicalNode.uid ?? prev.uid ?? null;
        const nextPtr = canonicalPtr ?? prev.ptr ?? null;
        if (prev.id === nextId && prev.uid === nextUid && prev.ptr === nextPtr) {
          return prev;
        }
        return { id: nextId, uid: nextUid, ptr: nextPtr };
      });
    }

    lastHydratedRef.current = serialized;
  }, [graphSnapshot, selectedIdentity.id, selectedIdentity.uid, selectedPtr, setEdges, setNodes, setSelectedIdentity, setSelectedPtr]);

  useEffect(() => {
    const buildIndex = (selector: (node: GraphNodeSnapshot) => string | null): Record<string, number> => {
      const next: Record<string, number> = {};
      for (const node of graphSnapshot.nodes ?? []) {
        if (!node) continue;
        const key = selector(node);
        if (!key) continue;
        if (typeof node.ptr === 'number' && Number.isFinite(node.ptr)) {
          next[key] = node.ptr;
        }
      }
      return next;
    };

    const nextById = buildIndex((node) => (node.id && node.id.length ? node.id : null));
    const nextByUid = buildIndex((node) => (node.uid && node.uid.length ? node.uid : null));

    setNodePtrById((current) => {
      const currentKeys = Object.keys(current);
      const nextKeys = Object.keys(nextById);
      if (currentKeys.length === nextKeys.length && nextKeys.every((key) => current[key] === nextById[key])) {
        return current;
      }
      return nextById;
    });

    setNodePtrByUid((current) => {
      const currentKeys = Object.keys(current);
      const nextKeys = Object.keys(nextByUid);
      if (currentKeys.length === nextKeys.length && nextKeys.every((key) => current[key] === nextByUid[key])) {
        return current;
      }
      return nextByUid;
    });
  }, [graphSnapshot.nodes, setNodePtrById, setNodePtrByUid]);

  useEffect(() => {
    if (selectedPtr == null) {
      return;
    }

    const node = graphSnapshot.nodes.find((entry) => {
      if (!entry) {
        return false;
      }
      if (entry.ptr === selectedPtr) {
        return true;
      }
      if (entry.id && entry.id === String(selectedPtr)) {
        return true;
      }
      return false;
    });

    if (!node) {
      return;
    }

    setSelectedIdentity((prev) => {
      const nextId = node.id ?? prev.id ?? String(node.ptr ?? selectedPtr);
      const nextUid = node.uid ?? prev.uid ?? null;
      if (prev.id === nextId && prev.uid === nextUid && prev.ptr === selectedPtr) {
        return prev;
      }
      return { id: nextId, uid: nextUid, ptr: selectedPtr };
    });
  }, [graphSnapshot.nodes, selectedPtr, setSelectedIdentity]);

  useEffect(() => {
    if (!selectedIdentity.id && !selectedIdentity.uid) {
      return;
    }

    const canonicalPtrFromUid = selectedIdentity.uid ? ptrByUid[selectedIdentity.uid] : undefined;
    const canonicalPtrFromId = selectedIdentity.id ? ptrById[selectedIdentity.id] : undefined;
    const canonicalPtr = canonicalPtrFromUid ?? canonicalPtrFromId;

    if (!canonicalPtr || canonicalPtr === selectedIdentity.ptr) {
      return;
    }

    setSelectedIdentity((prev) => {
      if (prev.id !== selectedIdentity.id || prev.uid !== selectedIdentity.uid) {
        return prev;
      }
      return { ...prev, ptr: canonicalPtr };
    });

    setSelectedPtr((current) => (current === canonicalPtr ? current : canonicalPtr));
  }, [ptrById, ptrByUid, selectedIdentity.id, selectedIdentity.uid, selectedIdentity.ptr, setSelectedIdentity, setSelectedPtr]);

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
    <div
      ref={containerRef}
      style={{ width: '100%', height: '100%', position: 'relative' }}
      onDrop={onDrop}
      onDragOver={onDragOver}
    >
      <ReactFlow
        nodes={nodes}
        edges={edges}
        nodeTypes={nodeTypes}
        onConnect={onConnect}
        onNodesChange={handleNodesChange}
        onEdgesChange={onEdgesChange}
        onSelectionChange={onSelectionChange}
        onEdgeClick={handleEdgeClick}
        onNodeDoubleClick={handleNodeDoubleClick}
        onPaneClick={closeActionBubbles}
        onMoveStart={closeActionBubbles}
        fitView
      >
        <MiniMap />
        <Controls />
        <Background />
      </ReactFlow>
      {edgeBubble ? <DeleteBubble position={edgeBubble.position} onConfirm={handleEdgeDelete} /> : null}
      {nodeBubble ? <DeleteBubble position={nodeBubble.position} onConfirm={handleNodeDelete} /> : null}
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
