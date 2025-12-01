import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { useRecoilState, useRecoilValue, useSetRecoilState } from 'recoil';
import { PropertyEditor } from '@/app/components/PropertyEditor';
import type { PropertySpec } from '@/types/properties';
import { graphSnapshotAtom } from '@state/atoms/project';
import { nodePropertyCacheAtom, nodePtrByIdAtom, nodePtrByUidAtom, selectedNodeIdentityAtom, selectedNodePtrAtom } from '@state/atoms/editor';
import { engine } from '@/app/services/ipc/cefBridge';
import { useGraphCommands } from '@state/hooks/useGraphCommands';

interface PropertySchema {
  properties: PropertySpec[];
}

const sanitizePropertyList = (schema: PropertySchema | null | undefined): PropertySpec[] => {
  if (!schema || !Array.isArray(schema.properties)) {
    return [];
  }

  return schema.properties
    .filter((prop): prop is PropertySpec => Boolean(prop && typeof prop.name === 'string'))
    .map((prop, index) => ({
      ...prop,
      index: typeof prop.index === 'number' ? prop.index : index,
      label: prop.label ?? prop.name,
    }));
};

const inferTypeFromValue = (value: unknown): PropertySpec['type'] => {
  if (typeof value === 'boolean') return 'bool';
  if (typeof value === 'number') return Number.isInteger(value) ? 'int' : 'float';
  if (typeof value === 'string') return 'string';
  if (Array.isArray(value)) {
    if (value.length === 2) return 'vec2';
    if (value.length === 3) return 'vec3';
    if (value.length === 4) return 'vec4';
    return 'string';
  }
  if (value && typeof value === 'object') return 'string';
  return 'string';
};

const buildSchemaFromDict = (dict: Record<string, unknown>): PropertySpec[] => {
  const entries = Object.entries(dict);
  return entries.map(([name, value], index) => ({
    name,
    label: name,
    index,
    type: inferTypeFromValue(value),
    value,
    default: value,
  }));
};

const loosenJson = (raw: string): any | null => {
  const trimmed = raw.trim();
  if (!trimmed.length) {
    return null;
  }

  try {
    return JSON.parse(trimmed);
  } catch {
    // fallthrough to relaxed parsing
  }

  const withQuotedKeys = trimmed
    .replace(/([,{]\s*)([A-Za-z_][A-Za-z0-9_]*)\s*:/g, '$1"$2":')
    .replace(/:\s*(?=[,}])/g, ': null')
    .replace(/,\s*([}\]])/g, '$1');

  try {
    return JSON.parse(withQuotedKeys);
  } catch {
    return null;
  }
};

const coercePropertySchema = (payload: unknown): PropertySpec[] | null => {
  if (payload == null) {
    return null;
  }

  if (Array.isArray((payload as PropertySchema).properties)) {
    return sanitizePropertyList(payload as PropertySchema);
  }

  if (typeof payload === 'object') {
    return buildSchemaFromDict(payload as Record<string, unknown>);
  }

  if (typeof payload === 'string') {
    const parsed = loosenJson(payload);
    if (parsed == null) {
      return null;
    }
    return coercePropertySchema(parsed);
  }

  return null;
};

export function InspectorPanel() {
  const graphSnapshot = useRecoilValue(graphSnapshotAtom);
  const setSelectedPtr = useSetRecoilState(selectedNodePtrAtom);
  const selectedIdentity = useRecoilValue(selectedNodeIdentityAtom);
  const ptrById = useRecoilValue(nodePtrByIdAtom);
  const ptrByUid = useRecoilValue(nodePtrByUidAtom);
  const legacySelectedPtr = useRecoilValue(selectedNodePtrAtom);
  const selectedPtr = useMemo(() => {
    if (selectedIdentity.uid) {
      const canonical = ptrByUid[selectedIdentity.uid];
      if (typeof canonical === 'number') {
        return canonical;
      }
    }
    if (selectedIdentity.id) {
      const canonical = ptrById[selectedIdentity.id];
      if (typeof canonical === 'number') {
        return canonical;
      }
    }
    if (typeof selectedIdentity.ptr === 'number') {
      return selectedIdentity.ptr;
    }
    return typeof legacySelectedPtr === 'number' ? legacySelectedPtr : null;
  }, [legacySelectedPtr, ptrById, ptrByUid, selectedIdentity]);
  const [propertyCache, setPropertyCache] = useRecoilState(nodePropertyCacheAtom);
  const missingNodePtrRef = useRef<number | null>(null);
  const [lastKnownNodeId, setLastKnownNodeId] = useState<string | null>(null);
  const [lastKnownNodeUid, setLastKnownNodeUid] = useState<string | null>(null);
  const [properties, setProperties] = useState<PropertySpec[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [nameDraft, setNameDraft] = useState('');
  const { updateNodeLabel } = useGraphCommands();

  const selectedNode = useMemo(() => {
    if (selectedIdentity.uid) {
      const match = graphSnapshot.nodes.find((node) => node.uid && node.uid === selectedIdentity.uid);
      if (match) {
        return match;
      }
    }
    if (selectedPtr != null) {
      const match = graphSnapshot.nodes.find((node) => node.ptr === selectedPtr || node.id === String(selectedPtr));
      if (match) {
        return match;
      }
    }
    if (selectedIdentity.id) {
      return graphSnapshot.nodes.find((node) => node.id === selectedIdentity.id) ?? null;
    }
    return null;
  }, [graphSnapshot.nodes, selectedIdentity.id, selectedIdentity.uid, selectedPtr]);
  const cacheKey = useMemo(() => {
    if (selectedIdentity.uid) {
      return `uid:${selectedIdentity.uid}`;
    }
    if (lastKnownNodeUid) {
      return `uid:${lastKnownNodeUid}`;
    }
    if (selectedIdentity.id) {
      return `id:${selectedIdentity.id}`;
    }
    if (lastKnownNodeId) {
      return `id:${lastKnownNodeId}`;
    }
    if (selectedPtr != null) {
      return `ptr:${selectedPtr}`;
    }
    return null;
  }, [lastKnownNodeId, lastKnownNodeUid, selectedIdentity.id, selectedIdentity.uid, selectedPtr]);
  const liveProps = selectedNode?.props ?? null;
  const livePropPairs = useMemo(() => {
    if (!liveProps) return [];
    return Object.entries(liveProps)
      .filter(([_, value]) => ['string', 'number', 'boolean'].includes(typeof value))
      .slice(0, 6);
  }, [liveProps]);

  useEffect(() => {
    if (selectedNode?.id) {
      setLastKnownNodeId((current) => (current === selectedNode.id ? current : selectedNode.id));
    }
    if (selectedNode && typeof selectedNode.uid === 'string' && selectedNode.uid.length) {
      const nextUid = selectedNode.uid;
      setLastKnownNodeUid((current) => (current === nextUid ? current : nextUid));
    }
  }, [selectedNode]);

  useEffect(() => {
    setProperties((current) => {
      if (!cacheKey) {
        return current.length ? [] : current;
      }
      const cached = propertyCache[cacheKey];
      if (cached && current !== cached) {
        return cached;
      }
      if (!cached && current.length) {
        return [];
      }
      return current;
    });
  }, [cacheKey, propertyCache]);

  useEffect(() => {
    if (!selectedNode) {
      setNameDraft('');
      return;
    }
    setNameDraft(selectedNode.label ?? selectedNode.type ?? '');
  }, [selectedNode]);

  const handleNameChange = useCallback(
    async (value: string) => {
      setNameDraft(value);
      if (!selectedNode) return;

      const normalized = value?.trim()?.length ? value : selectedNode.type ?? 'Node';
      try {
        await updateNodeLabel(selectedNode.id ?? selectedNode.ptr ?? null, normalized);
      } catch (error) {
        console.warn('[Inspector] Failed to sync label with engine', error);
      }
    },
    [selectedNode, updateNodeLabel],
  );

  useEffect(() => {
    if (selectedPtr == null && !selectedIdentity.id && !selectedIdentity.uid) {
      missingNodePtrRef.current = null;
      return;
    }

    const stillExists = graphSnapshot.nodes.some((node) => {
      if (!node) return false;
      if (selectedPtr != null && (node.ptr === selectedPtr || node.id === String(selectedPtr))) {
        return true;
      }
      if (selectedIdentity.uid && node.uid === selectedIdentity.uid) {
        return true;
      }
      if (selectedIdentity.id && node.id === selectedIdentity.id) {
        return true;
      }
      return false;
    });

    if (!stillExists && selectedPtr != null) {
      missingNodePtrRef.current = selectedPtr;
      setSelectedPtr((current) => (current === selectedPtr ? null : current));
    } else if (stillExists) {
      missingNodePtrRef.current = null;
    }
  }, [graphSnapshot.nodes, selectedIdentity.id, selectedIdentity.uid, selectedPtr, setSelectedPtr]);

  const canonicalPtr = useMemo(() => {
    if (selectedNode && typeof selectedNode.ptr === 'number' && Number.isFinite(selectedNode.ptr)) {
      return selectedNode.ptr;
    }
    return null;
  }, [selectedNode]);

  useEffect(() => {
    let cancelled = false;

    const effectivePtr = canonicalPtr ?? selectedPtr;

    if (canonicalPtr != null && selectedPtr !== canonicalPtr) {
      setSelectedPtr(canonicalPtr);
      setLoading(false);
      setError(null);
      return () => {
        cancelled = true;
      };
    }

    if (effectivePtr == null) {
      if (!selectedIdentity.id && !selectedIdentity.uid) {
        setProperties([]);
        setError(null);
      }
      setLoading(false);
      return;
    }

    if (missingNodePtrRef.current === effectivePtr) {
      setProperties([]);
      setError(null);
      setLoading(false);
      return;
    }

    setLoading(true);
    setError(null);

    (async () => {
      try {
        const schema = await engine.getNodeProperties(effectivePtr);

        if (cancelled) return;

        const parsedProps = coercePropertySchema(schema ?? {}) ?? [];

        setProperties(parsedProps);
        setError(null);

        if (cacheKey) {
          setPropertyCache((previous) => {
            const existing = previous[cacheKey];
            if (existing === parsedProps) {
              return previous;
            }
            return { ...previous, [cacheKey]: parsedProps };
          });
        }
      } catch (e: any) {
        if (!cancelled) {
          const message = e?.message ?? e;
          setError(`Failed to load properties: ${message}`);
          setProperties([]);
          const normalized = typeof message === 'string' ? message : String(message);
          if (normalized?.toLowerCase().includes('node not found')) {
            missingNodePtrRef.current = effectivePtr;
            setSelectedPtr((current) => (current === effectivePtr ? null : current));
          }
        }
      } finally {
        if (!cancelled) {
          setLoading(false);
        }
      }
    })();

    return () => {
      cancelled = true;
    };
  }, [cacheKey, canonicalPtr, selectedIdentity.id, selectedIdentity.uid, selectedPtr, setPropertyCache, setSelectedPtr]);

  const showLoadingState = loading && properties.length === 0;

  if (selectedPtr == null && !selectedIdentity.id && !selectedIdentity.uid) {
    return (
      <div className="h-full flex items-center justify-center text-gray-500">
        <div className="text-center">
          <div className="text-4xl mb-2">⚙️</div>
          <div>Select a node to view properties</div>
        </div>
      </div>
    );
  }

  if (selectedPtr == null) {
    return (
      <div className="h-full flex items-center justify-center text-gray-500">
        <div className="text-center">
          <div className="text-4xl mb-2">⌛</div>
          <div>Waiting for node handle…</div>
        </div>
      </div>
    );
  }

  if (showLoadingState) {
    return (
      <div className="h-full flex items-center justify-center text-gray-500">
        <div className="text-center">
          <div className="animate-spin text-2xl mb-2">⚙️</div>
          <div>Loading properties...</div>
        </div>
      </div>
    );
  }

  if (error && properties.length === 0) {
    return (
      <div className="p-4">
        <div className="text-red-400 text-sm mb-3">
          <strong>Error:</strong> {error}
        </div>
        <details className="text-xs text-gray-500">
          <summary className="cursor-pointer hover:text-gray-400">Debug Info</summary>
          <pre className="mt-2 p-2 bg-gray-800 rounded text-xs overflow-auto">Selected Node: #{selectedPtr}</pre>
        </details>
      </div>
    );
  }

  return (
    <div className="h-full overflow-auto">
      {error && properties.length > 0 && (
        <div className="px-4 py-2 text-xs text-red-300 bg-red-500/10 border-b border-red-500/20">
          Failed to refresh properties: {error}
        </div>
      )}
      {selectedNode && (
        <div className="p-4 border-b border-ui-border/50 text-xs text-gray-400 space-y-2">
          <div>
            <label className="text-[11px] uppercase tracking-wide text-gray-500">Node Name</label>
            <input
              type="text"
              value={nameDraft}
              onChange={(event) => handleNameChange(event.target.value)}
              className="mt-1 w-full rounded border border-ui-border bg-black/30 px-2 py-1 text-sm text-gray-100 focus:border-ui-accent focus:outline-none"
            />
          </div>
          <div className="grid grid-cols-2 gap-2">
            <div>
              <div className="text-[10px] uppercase text-gray-500">ID</div>
              <div className="text-sm text-gray-200">{selectedNode.id}</div>
            </div>
            {selectedNode.ptr != null && (
              <div>
                <div className="text-[10px] uppercase text-gray-500">Ptr</div>
                <div className="text-sm text-gray-200">{selectedNode.ptr}</div>
              </div>
            )}
            <div>
              <div className="text-[10px] uppercase text-gray-500">Type</div>
              <div className="text-sm text-gray-200">{selectedNode.type}</div>
            </div>
            <div>
              <div className="text-[10px] uppercase text-gray-500">Pos.</div>
              <div className="text-sm text-gray-200">
                {Math.round(selectedNode.position?.x ?? 0)}×{Math.round(selectedNode.position?.y ?? 0)}
              </div>
            </div>
          </div>
          {livePropPairs.length > 0 && (
            <div className="pt-2 border-t border-ui-border/30 grid grid-cols-2 gap-2">
              {livePropPairs.map(([key, value]) => (
                <div key={key} className="flex flex-col gap-0.5">
                  <div className="text-[10px] uppercase text-gray-500">{key}</div>
                  <div className="text-sm text-gray-200 truncate" title={String(value)}>
                    {typeof value === 'boolean' ? (value ? 'True' : 'False') : String(value)}
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      )}
      <PropertyEditor nodePtr={selectedPtr} properties={properties} className="h-full" liveValues={liveProps} />
    </div>
  );
}
