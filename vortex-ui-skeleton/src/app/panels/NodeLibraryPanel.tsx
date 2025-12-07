import { useCallback, useEffect, useMemo, useState } from 'react';
import type { DragEvent } from 'react';
import { engine } from '@/app/services/ipc/cefBridge';
import { useGraphCommands } from '@state/hooks/useGraphCommands';

const SHOW_NODE_DEBUG = Boolean(import.meta.env && import.meta.env.DEV);

export function NodeLibraryPanel() {
  const [query, setQuery] = useState('');
  const [types, setTypes] = useState<string[]>([]);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [actionError, setActionError] = useState<string | null>(null);
  const { createNode } = useGraphCommands();

  useEffect(() => {
    let alive = true;
    (async () => {
      try {
        setLoadError(null);
        const res = await engine.listNodeTypes();
        if (SHOW_NODE_DEBUG) {
          console.debug('[getNodeTypes] raw ->', res);
        }
        if (SHOW_NODE_DEBUG) {
          console.debug('[getNodeTypes] parsed list ->', res);
        }
        if (alive) setTypes(res);
      } catch (e: any) {
        console.error('[getNodeTypes] error ->', e);
        if (alive) setLoadError(`Bridge error: ${e?.message ?? String(e)}`);
      }
    })();
    return () => {
      alive = false;
    };
  }, []);

  const handleCreate = useCallback(
    async (typeName: string) => {
      try {
        setActionError(null);
        await createNode(typeName);
      } catch (error) {
        console.error('[NodeLibrary] Failed to create node', error);
        setActionError(error instanceof Error ? error.message : 'Failed to create node');
      }
    },
    [createNode],
  );

  const filtered = useMemo(() => types.filter((t) => t.toLowerCase().includes(query.toLowerCase())), [types, query]);

  const onDragStart = useCallback((event: DragEvent<HTMLButtonElement>, nodeType: string) => {
    event.dataTransfer.setData('application/x-vortex-node-type', nodeType);
    event.dataTransfer.effectAllowed = 'move';
  }, []);

  return (
    <div className="p-3" style={{ height: '100%', overflow: 'auto' }}>

      {loadError && (
        <div className="text-red-400 text-xs mb-3 p-2 rounded" style={{ background: '#3a1a1a' }}>
          Error: {loadError}
        </div>
      )}

      {actionError && (
        <div className="text-amber-300 text-xs mb-3 p-2 rounded" style={{ background: '#3a2a1a' }}>
          {actionError}
        </div>
      )}

      <input
        type="text"
        placeholder="Search nodes..."
        value={query}
        onChange={(e) => setQuery(e.target.value)}
        style={{
          width: '100%',
          marginBottom: 12,
          padding: '8px 10px',
          background: 'rgba(8, 16, 32, 0.95)',
          border: '1px solid rgba(143, 211, 255, 0.25)',
          borderRadius: 12,
          color: '#f3f6ff',
          fontSize: 12,
          boxShadow: '0 10px 25px rgba(2, 6, 14, 0.6)',
        }}
      />

      <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
        {filtered.map((t) => (
          <button
            key={t}
            onClick={() => handleCreate(t)}
            draggable
            onDragStart={(e) => onDragStart(e, t)}
            className="
              flex items-center gap-3 p-3 rounded-2xl text-left transition-all duration-200
              text-sm cursor-grab active:cursor-grabbing
            "
            style={
              {
                background: 'linear-gradient(135deg, rgba(8, 16, 32, 0.95), rgba(5, 10, 22, 0.95))',
                border: '1px solid rgba(143, 211, 255, 0.18)',
                color: '#f3f6ff',
                boxShadow: '0 15px 40px rgba(2, 6, 14, 0.55)',
              }
            }
            title={`Drag to add ${t} node or click to create at default position`}
          >
            <div className="w-7 h-7 rounded-lg bg-cyan-500/15 text-cyan-300 flex items-center justify-center text-xs">
              {t.includes('Input') ? '📥' : t.includes('Output') ? '📤' : '⚙️'}
            </div>
            <span className="flex-1">{t}</span>
            <div className="text-xs text-gray-500">⋮⋮</div>
          </button>
        ))}
      </div>
    </div>
  );
}
