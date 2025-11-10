import { useEffect, useMemo, useState } from 'react';
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

  return (
    <div style={{ padding: 10 }}>
      <input
        placeholder="Search..."
        value={query}
        onChange={e => setQuery(e.target.value)}
        style={{ width: '100%' }}
      />
      {err && <div style={{ color: '#e57373', marginTop: 6 }}>{err}</div>}

      <div style={{ marginTop: 8, display: 'grid', gap: 6 }}>
        {filtered.map(t => (
          <button key={t} onClick={() => onCreate(t)}
                  style={{ padding: '6px 8px', textAlign: 'left' }}>
            {t}
          </button>
        ))}
      </div>
    </div>
  );
}
