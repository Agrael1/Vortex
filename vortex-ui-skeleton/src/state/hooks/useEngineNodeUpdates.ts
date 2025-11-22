import { useEffect } from 'react';
import { useSetRecoilState } from 'recoil';
import type { SetterOrUpdater } from 'recoil';
import { graphSnapshotAtom } from '@state/atoms/project';
import type { GraphNodeSnapshot, GraphSnapshot } from '@state/types';
import { engine as nativeEngine } from '@/bridge/engine';
import { Vortex } from '@/bridge/vortex';

type PropertySchema = {
  properties?: {
    name?: string;
    index?: number;
  }[];
};

type Position = { x: number; y: number };

const propertyNameCache = new Map<number, Map<number, string>>();
const pendingPropertyFetch = new Map<number, Promise<Map<number, string>>>();

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

const cachePropertyNames = async (nodePtr: number): Promise<Map<number, string>> => {
  if (propertyNameCache.has(nodePtr)) {
    return propertyNameCache.get(nodePtr)!;
  }

  if (pendingPropertyFetch.has(nodePtr)) {
    return pendingPropertyFetch.get(nodePtr)!;
  }

  const fetchPromise = (async () => {
    try {
      const raw = await Vortex.getNodeProperties(nodePtr);
      const schema = parseSchema(raw);
      const map = new Map<number, string>();

      for (const prop of schema.properties ?? []) {
        if (!prop) continue;
        if (typeof prop.index !== 'number') continue;
        if (typeof prop.name !== 'string' || prop.name.length === 0) continue;
        map.set(prop.index, prop.name);
      }

      propertyNameCache.set(nodePtr, map);
      return map;
    } catch (error) {
      console.warn('[NodeUpdateSync] Failed to fetch properties for node', { nodePtr, error });
      const empty = new Map<number, string>();
      propertyNameCache.set(nodePtr, empty);
      return empty;
    } finally {
      pendingPropertyFetch.delete(nodePtr);
    }
  })();

  pendingPropertyFetch.set(nodePtr, fetchPromise);
  return fetchPromise;
};

const resolvePropertyName = async (nodePtr: number, propIndex: number): Promise<string | null> => {
  if (!Number.isFinite(nodePtr) || !Number.isFinite(propIndex)) {
    return null;
  }

  const map = await cachePropertyNames(nodePtr);
  return map.get(propIndex) ?? null;
};

type NodeMutator = (node: GraphNodeSnapshot) => GraphNodeSnapshot | null;

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
      const nextProps = { ...(node.props ?? {}) };
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

export function useEngineNodeUpdates() {
  const setGraphSnapshot = useSetRecoilState(graphSnapshotAtom);

  useEffect(() => {
    if (typeof nativeEngine.onNodeUpdate !== 'function') {
      return;
    }

    let disposed = false;

    const unsubscribe = nativeEngine.onNodeUpdate(async (nodePtr, propIndex, value) => {
      try {
        const propName = await resolvePropertyName(nodePtr, propIndex);
        if (disposed || !propName) {
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
    });

    return () => {
      disposed = true;
      if (typeof unsubscribe === 'function') {
        unsubscribe();
      }
    };
  }, [setGraphSnapshot]);
}
