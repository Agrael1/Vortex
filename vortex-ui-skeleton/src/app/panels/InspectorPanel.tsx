import { useCallback, useEffect, useMemo, useState } from 'react';
import { useRecoilCallback, useRecoilValue } from 'recoil';
import { Vortex } from '@/bridge/vortex';
import { PropertyEditor, PropertySpec } from '@/app/components/PropertyEditor';
import { graphSnapshotAtom } from '@state/atoms/project';
import { selectedNodePtrAtom } from '@state/atoms/editor';

interface PropertySchema {
  properties: PropertySpec[];
}

export function InspectorPanel() {
  const selectedPtr = useRecoilValue(selectedNodePtrAtom);
  const graphSnapshot = useRecoilValue(graphSnapshotAtom);
  const [properties, setProperties] = useState<PropertySpec[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [nameDraft, setNameDraft] = useState('');

  const selectedNode = useMemo(() => {
    if (!selectedPtr) return null;
    return graphSnapshot.nodes.find((node) => node.ptr === selectedPtr || node.id === String(selectedPtr)) ?? null;
  }, [graphSnapshot.nodes, selectedPtr]);
  const liveProps = selectedNode?.props ?? null;
  const livePropPairs = useMemo(() => {
    if (!liveProps) return [];
    return Object.entries(liveProps)
      .filter(([_, value]) => ['string', 'number', 'boolean'].includes(typeof value))
      .slice(0, 6);
  }, [liveProps]);

  useEffect(() => {
    if (!selectedNode) {
      setNameDraft('');
      return;
    }
    setNameDraft(selectedNode.label ?? selectedNode.type ?? '');
  }, [selectedNode]);

  const updateNodeLabel = useRecoilCallback(
    ({ set }) =>
      (ptr: number, label: string) => {
        set(graphSnapshotAtom, (prev) => ({
          ...prev,
          nodes: prev.nodes.map((node) =>
            node.ptr === ptr || node.id === String(ptr) ? { ...node, label: label?.trim().length ? label : node.type } : node,
          ),
        }));
      },
    [],
  );

  const handleNameChange = useCallback(
    async (value: string) => {
      setNameDraft(value);
      if (!selectedPtr) return;

      const normalized = value?.trim()?.length ? value : (selectedNode?.type ?? 'Node');
      updateNodeLabel(selectedPtr, normalized);

      try {
        await Vortex.setNodeProperty(selectedPtr, 'label', normalized);
      } catch (error) {
        console.warn('[Inspector] Failed to sync label with engine', error);
      }
    },
    [selectedNode?.type, selectedPtr, updateNodeLabel],
  );

  useEffect(() => {
    let cancelled = false;

    if (!selectedPtr) {
      setProperties([]);
      setError(null);
      return;
    }

    setLoading(true);
    setError(null);

    (async () => {
      try {
        const raw = await Vortex.getNodeProperties(selectedPtr);

        if (cancelled) return;

        let parsedProps: PropertySchema;

        if (typeof raw === 'string') {
          try {
            parsedProps = JSON.parse(raw);
          } catch {
            // If parsing fails, treat it as raw text and show error
            setError(`Invalid JSON response: ${raw}`);
            setProperties([]);
            return;
          }
        } else {
          parsedProps = raw as PropertySchema;
        }

        if (parsedProps && parsedProps.properties && Array.isArray(parsedProps.properties)) {
          setProperties(parsedProps.properties);
        } else {
          setError('Invalid properties format received from engine');
          setProperties([]);
        }
      } catch (e: any) {
        if (!cancelled) {
          setError(`Failed to load properties: ${e?.message ?? e}`);
          setProperties([]);
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
  }, [selectedPtr]);

  if (!selectedPtr) {
    return (
      <div className="h-full flex items-center justify-center text-gray-500">
        <div className="text-center">
          <div className="text-4xl mb-2">⚙️</div>
          <div>Select a node to view properties</div>
        </div>
      </div>
    );
  }

  if (loading) {
    return (
      <div className="h-full flex items-center justify-center text-gray-500">
        <div className="text-center">
          <div className="animate-spin text-2xl mb-2">⚙️</div>
          <div>Loading properties...</div>
        </div>
      </div>
    );
  }

  if (error) {
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
      {selectedNode && (
        <div className="p-4 border-b border-ui-border/50 text-xs text-gray-400 space-y-2">
          <div>
            <label className="text-[11px] uppercase tracking-wide text-gray-500">Имя узла</label>
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
              <div className="text-[10px] uppercase text-gray-500">Тип</div>
              <div className="text-sm text-gray-200">{selectedNode.type}</div>
            </div>
            <div>
              <div className="text-[10px] uppercase text-gray-500">Поз.</div>
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
