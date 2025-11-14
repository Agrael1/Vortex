import { useCallback, useEffect, useMemo, useState } from 'react';
import { Vortex } from '@/bridge/vortex';

type Props = { onCreate: (typeName: string) => void };

export function NodeLibraryPanel({ onCreate }: Props) {
  const [query, setQuery] = useState('');
  const [types, setTypes] = useState<string[]>([]);
  const [err, setErr] = useState<string | null>(null);

  useEffect(() => {
    let alive = true;
    (async () => {
      try {
        setErr(null);
        const res = await Vortex.getNodeTypes();
        console.debug('[getNodeTypes] raw ->', res);
        
        let list: string[] = [];
        
        if (Array.isArray(res)) {
          // If array of strings
          list = res.map(String);
        } else if (res && typeof res === 'object') {
          if (Array.isArray(res.types)) {
            // If object with types field
            list = res.types.map(String);
          } else {
            // If object dictionary {nodeName: nodeInfo} - take keys
            list = Object.keys(res);
          }
        }
        
        console.debug('[getNodeTypes] parsed list ->', list);
        if (alive) setTypes(list);
      } catch (e: any) {
        console.error('[getNodeTypes] error ->', e);
        if (alive) setErr(`Bridge error: ${e?.message ?? String(e)}`);
      }
    })();
    return () => { alive = false; };
  }, []);

  const filtered = useMemo(
    () => types.filter(t => t.toLowerCase().includes(query.toLowerCase())),
    [types, query]
  );

  const onDragStart = useCallback((event: React.DragEvent, nodeType: string) => {
    event.dataTransfer.setData('application/x-vortex-node-type', nodeType);
    event.dataTransfer.effectAllowed = 'move';
  }, []);

  return (
    <div className="p-3" style={{ height: '100%', overflow: 'auto' }}>
      <div className="text-sm font-semibold mb-3" style={{ color: '#ddd' }}>Node Library</div>
      
      {err && (
        <div className="text-red-400 text-xs mb-3 p-2 rounded" style={{ background: '#3a1a1a' }}>
          Error: {err}
        </div>
      )}
      
      <input
        type="text"
        placeholder="Search nodes..."
        value={query}
        onChange={e => setQuery(e.target.value)}
        style={{
          width: '100%', marginBottom: 12, padding: '6px 8px',
          background: '#1a1a1a', border: '1px solid #333', borderRadius: 4,
          color: '#ddd', fontSize: 12
        }}
      />
      
      <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
        {filtered.map(t => (
          <button 
            key={t} 
            onClick={() => onCreate(t)}
            draggable
            onDragStart={(e) => onDragStart(e, t)}
            className="
              flex items-center gap-2 p-2 rounded-md text-left transition-all duration-200
              bg-gray-800/50 hover:bg-gray-700/70 border border-gray-600 hover:border-gray-500
              text-gray-200 text-sm cursor-grab active:cursor-grabbing
              hover:shadow-md
            "
            title={`Drag to add ${t} node or click to create at default position`}
          >
            <div className="w-6 h-6 rounded bg-blue-500/20 text-blue-400 flex items-center justify-center text-xs">
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
