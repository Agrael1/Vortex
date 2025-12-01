import { useEffect, useRef } from 'react';
import { useRecoilValue, useSetRecoilState } from 'recoil';
import type { SetterOrUpdater } from 'recoil';
import { graphSnapshotAtom } from '@state/atoms/project';
import { nodePtrByIdAtom, nodePtrByUidAtom } from '@state/atoms/editor';
import type { GraphNodeSnapshot, GraphSnapshot } from '@state/types';
import { engine, type NodeCreatedPayload } from '@/app/services/ipc/cefBridge';

type PropertySchema = {
  properties?: {
    name?: string;
    index?: number;
  }[];
};

type Position = { x: number; y: number };

type NodeUpdatePayload = {
  nodePtr: number;
  propIndex: number;
  value: unknown;
};

type PropertyCacheEntry = {
  map: Map<number, string>;
  fetchedAt: number;
};

type NodeMutator = (node: GraphNodeSnapshot) => GraphNodeSnapshot | null;

const PROPERTY_CACHE_TTL_MS = 60_000;

const propertyNameCache = new Map<number, PropertyCacheEntry>();
const pendingFetches = new Map<number, Promise<Map<number, string>>>();

const parseSchema = (raw: unknown): PropertySchema => {
  if (!raw) return {};
  if (typeof raw === 'string') {
    try {
      return JSON.parse(raw);
    } catch (error) {
      console.warn('[NodeUpdateSync] Failed to parse property schema', error);
      return {};
    }
  }
  if (typeof raw === 'object') {
    return raw as PropertySchema;
  }
  return {};
};

const storePropertyCache = (nodePtr: number, map: Map<number, string>) => {
  propertyNameCache.set(nodePtr, { map, fetchedAt: Date.now() });
  return map;
};

const buildPropertyMap = (schema: unknown) => {
  const parsed = parseSchema(schema);
  const map = new Map<number, string>();
  for (const prop of parsed.properties ?? []) {
    if (!prop) continue;
    if (typeof prop.index !== 'number') continue;
    if (typeof prop.name !== 'string' || prop.name.length === 0) continue;
    map.set(prop.index, prop.name);
  }
  return map;
};

const primePropertyCache = async (nodePtr: number) => {
  try {
    const schema = await engine.getNodeProperties(nodePtr);
    return storePropertyCache(nodePtr, buildPropertyMap(schema));
  } catch (error) {
    console.warn('[NodeUpdateSync] Failed to fetch property schema', { nodePtr, error });
    return storePropertyCache(nodePtr, new Map());
  }
};

const isEntryFresh = (entry: PropertyCacheEntry | undefined) => {
  if (!entry) {
    return false;
  }
  return Date.now() - entry.fetchedAt <= PROPERTY_CACHE_TTL_MS;
};

const toPosition = (value: unknown): Position | null => {
  if (!value || typeof value !== 'object') {
    return null;
  }

  const candidate = value as Record<string, unknown>;
  const x = Number(candidate.x);
  const y = Number(candidate.y);

  if (!Number.isFinite(x) || !Number.isFinite(y)) {
    return null;
  }

  return { x, y };
};

const mutateNode = (
  nodePtr: number,
  mutator: NodeMutator,
  setGraphSnapshot: SetterOrUpdater<GraphSnapshot>,
  options?: { createIfMissing?: boolean },
) => {
  const { createIfMissing = false } = options ?? {};
  setGraphSnapshot((prev) => {
    const nodes = prev.nodes ?? [];
    const targetId = String(nodePtr);
    let changed = false;
    let found = false;

    const nextNodes = nodes.map((node): GraphNodeSnapshot => {
      const matches = node.ptr === nodePtr || node.id === targetId;
      if (!matches) {
        return node;
      }
      found = true;
      const next = mutator(node);
      if (!next || next === node) {
        return node;
      }
      changed = true;
      return next;
    });

    if (!found && createIfMissing) {
      const draft: GraphNodeSnapshot = {
        id: targetId,
        type: 'Node',
        label: targetId,
        position: { x: 0, y: 0 },
        ptr: nodePtr,
        props: {},
      };
      const created = mutator(draft);
      if (created) {
        nextNodes.push(created);
        changed = true;
      }
    }

    if (!changed) {
      return prev;
    }

    return { ...prev, nodes: nextNodes };
  });
};

const storeNodeProp = (
  nodePtr: number,
  propName: string,
  value: unknown,
  setGraphSnapshot: SetterOrUpdater<GraphSnapshot>,
) => {
  mutateNode(
    nodePtr,
    (node) => {
      const nextProps = { ...(node.props ?? {}) } as Record<string, unknown>;
      if (Object.prototype.hasOwnProperty.call(nextProps, propName) && Object.is(nextProps[propName], value)) {
        return node;
      }
      nextProps[propName] = value;
      return { ...node, props: nextProps };
    },
    setGraphSnapshot,
    { createIfMissing: true },
  );
};

const fetchPropertyNames = async (nodePtr: number, options?: { force?: boolean }): Promise<Map<number, string>> => {
  const { force = false } = options ?? {};
  const entry = propertyNameCache.get(nodePtr);
  if (!force && isEntryFresh(entry)) {
    return entry!.map;
  }

  if (force) {
    pendingFetches.delete(nodePtr);
  }

  if (!pendingFetches.has(nodePtr)) {
    const task = primePropertyCache(nodePtr).finally(() => {
      if (pendingFetches.get(nodePtr) === task) {
        pendingFetches.delete(nodePtr);
      }
    });
    pendingFetches.set(nodePtr, task);
  }

  return pendingFetches.get(nodePtr)!;
};

const upsertNodeFromCreation = (
  payload: NodeCreatedPayload,
  setGraphSnapshot: SetterOrUpdater<GraphSnapshot>,
) => {
  const { ptr, id, uid, type, label, position, clientNodeId, props } = payload;
  if (!Number.isFinite(ptr) || (!id && !uid)) {
    return;
  }

  const normalizedId = id?.length ? id : clientNodeId ?? (uid || `node-${ptr}`);

  setGraphSnapshot((prev) => {
    const nodes = prev.nodes ?? [];
    const matchIndex = nodes.findIndex((node) => {
      if (!node) return false;
      if (uid && node.uid && node.uid === uid) return true;
      if (typeof node.ptr === 'number' && node.ptr === ptr) return true;
      if (node.id === normalizedId) return true;
      if (clientNodeId && node.id === clientNodeId) return true;
      return false;
    });

    const base: GraphNodeSnapshot = {
      id: normalizedId,
      uid: uid ?? null,
      type: type || 'Node',
      label: label || normalizedId,
      position: position ?? { x: 0, y: 0 },
      ptr,
      props: props ? { ...props } : undefined,
    };

    if (matchIndex >= 0) {
      const nextNodes = [...nodes];
      const existing = nextNodes[matchIndex];
      nextNodes[matchIndex] = {
        ...existing,
        ...base,
        uid: base.uid ?? existing?.uid ?? null,
        position: base.position ?? existing?.position,
        props: { ...(existing?.props ?? {}), ...(base.props ?? {}) },
      };
      return { ...prev, nodes: nextNodes };
    }

    return { ...prev, nodes: [...nodes, base] };
  });
};

const resolvePropertyName = async (nodePtr: number, propIndex: number): Promise<string | null> => {
  if (!Number.isFinite(nodePtr) || !Number.isFinite(propIndex)) {
    return null;
  }

  let map = await fetchPropertyNames(nodePtr);
  if (!map.has(propIndex)) {
    map = await fetchPropertyNames(nodePtr, { force: true });
  }
  return map.get(propIndex) ?? null;
};

const handleNodeUpdate = async (
  payload: NodeUpdatePayload,
  setGraphSnapshot: SetterOrUpdater<GraphSnapshot>,
) => {
  const { nodePtr, propIndex, value } = payload;
  try {
    const propName = await resolvePropertyName(nodePtr, propIndex);
    if (!propName) {
      return;
    }

    if (propName === 'position') {
      const nextPosition = toPosition(value);
      if (!nextPosition) {
        return;
      }

      mutateNode(
        nodePtr,
        (node) => {
          const prevPos = node.position ?? { x: 0, y: 0 };
          if (prevPos.x === nextPosition.x && prevPos.y === nextPosition.y) {
            return node;
          }
          return { ...node, position: nextPosition };
        },
        setGraphSnapshot,
        { createIfMissing: true },
      );
      storeNodeProp(nodePtr, propName, nextPosition, setGraphSnapshot);
      return;
    }

    if (propName === 'label' && typeof value === 'string') {
      const nextLabel = value.length ? value : nodePtr.toString();
      mutateNode(
        nodePtr,
        (node) => {
          if (node.label === nextLabel) {
            return node;
          }
          return { ...node, label: nextLabel };
        },
        setGraphSnapshot,
        { createIfMissing: true },
      );
      storeNodeProp(nodePtr, propName, value, setGraphSnapshot);
      return;
    }

    storeNodeProp(nodePtr, propName, value, setGraphSnapshot);
  } catch (error) {
    console.warn('[NodeUpdateSync] Failed to handle node update', { nodePtr, propIndex, error });
  }
};

export function useEngineNodeUpdates() {
  const setGraphSnapshot = useSetRecoilState(graphSnapshotAtom);
  const graphSnapshot = useRecoilValue(graphSnapshotAtom);
  const knownPtrsRef = useRef<Set<number>>(new Set());
  const setNodePtrById = useSetRecoilState(nodePtrByIdAtom);
  const setNodePtrByUid = useSetRecoilState(nodePtrByUidAtom);

  useEffect(() => {
    const unsubscribe = engine.on<NodeUpdatePayload>('node:update', async (payload) => {
      if (!payload) {
        return;
      }

      await handleNodeUpdate(payload, setGraphSnapshot);
    });

    return () => {
      unsubscribe?.();
      pendingFetches.clear();
    };
  }, [setGraphSnapshot]);

  useEffect(() => {
    const unsubscribe = engine.on<NodeCreatedPayload>('node:created', (payload) => {
      if (!payload) {
        return;
      }
      upsertNodeFromCreation(payload, setGraphSnapshot);
      if (payload.id && Number.isFinite(payload.ptr)) {
        setNodePtrById((current) => {
          if (current[payload.id!] === payload.ptr) {
            return current;
          }
          return { ...current, [payload.id!]: payload.ptr as number };
        });
      }

      if (payload.uid && Number.isFinite(payload.ptr)) {
        setNodePtrByUid((current) => {
          if (current[payload.uid!] === payload.ptr) {
            return current;
          }
          return { ...current, [payload.uid!]: payload.ptr as number };
        });
      }
    });

    return () => {
      unsubscribe?.();
    };
  }, [setGraphSnapshot, setNodePtrById, setNodePtrByUid]);

  useEffect(() => {
    const nextPtrs = new Set<number>();
    for (const node of graphSnapshot.nodes ?? []) {
      if (!node) continue;
      const { ptr } = node;
      if (typeof ptr === 'number' && Number.isFinite(ptr)) {
        nextPtrs.add(ptr);
      }
    }

    const prev = knownPtrsRef.current;
    if (prev.size) {
      prev.forEach((ptr) => {
        if (!nextPtrs.has(ptr)) {
          propertyNameCache.delete(ptr);
          pendingFetches.delete(ptr);
        }
      });
    }

    knownPtrsRef.current = nextPtrs;
  }, [graphSnapshot.nodes]);
}
