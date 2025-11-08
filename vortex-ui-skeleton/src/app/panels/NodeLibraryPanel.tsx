// src/app/panels/NodeLibraryPanel.tsx
import { useEffect, useMemo, useState } from 'react';
import { engine, type NodeTypeItem } from '@/bridge/engine';

type Props = { onCreate: (typeName: string, ptr: number) => void };

export function NodeLibraryPanel({ onCreate }: Props) {
  const [loading, setLoading] = useState(true);
  const [types, setTypes] = useState<Record<string, NodeTypeItem>>({});
  const [q, setQ] = useState('');

  useEffect(() => {
    let alive = true;
    (async () => {
      try {
        const dict = await engine.getNodeTypes();
        if (alive) setTypes(dict);
      } catch (e) {
        console.error('getNodeTypes failed', e);
        if (alive) setTypes({});
      } finally {
        if (alive) setLoading(false);
      }
    })();
    return () => {
      alive = false;
    };
  }, []);

  const grouped = useMemo(() => {
    const g: Record<string, NodeTypeItem[]> = {};
    for (const [name, info] of Object.entries(types)) {
      const cat = (info?.category || 'Other') as string;
      if (!g[cat]) g[cat] = [];
      g[cat].push({ ...info, type: info.type ?? name });
    }
    for (const k of Object.keys(g)) g[k].sort((a, b) => (a.type ?? '').localeCompare(b.type ?? ''));
    return g;
  }, [types]);

  const filtered = useMemo(() => {
    const term = q.trim().toLowerCase();
    if (!term) return grouped;
    const out: Record<string, NodeTypeItem[]> = {};
    for (const [cat, arr] of Object.entries(grouped)) {
      const hits = arr.filter((it) => (it.type ?? '').toLowerCase().includes(term));
      if (hits.length) out[cat] = hits;
    }
    return out;
  }, [q, grouped]);

  const handleCreate = async (typeName: string) => {
    try {
      const ptr = await engine.createNode(typeName);
      onCreate(typeName, ptr);
    } catch (e) {
      console.error('createNode failed', e);
    }
  };

  return (
    <div className="h-full flex flex-col text-xs text-gray-200">
      <div className="px-2 py-1 font-semibold text-gray-300">Nodes</div>

      <div className="p-2">
        <input
          className="w-full bg-[#151515] border border-[#2a2a2a] rounded px-2 py-1 outline-none"
          placeholder="Search..."
          value={q}
          onChange={(e) => setQ(e.target.value)}
        />
      </div>

      <div className="flex-1 overflow-auto px-2 pb-3 space-y-3">
        {loading && <div className="px-2 py-1 text-gray-400">Loading...</div>}

        {!loading && Object.keys(filtered).length === 0 && (
          <div className="px-2 py-1 text-gray-400">No nodes found</div>
        )}

        {!loading &&
          Object.entries(filtered).map(([cat, items]) => (
            <div key={cat}>
              <div className="px-2 py-1 uppercase text-[10px] tracking-wider text-gray-400">
                {cat}
              </div>
              <div className="space-y-1">
                {items.map((it) => (
                  <button
                    key={it.type}
                    onClick={() => handleCreate(String(it.type))}
                    className="w-full text-left px-2 py-1 rounded bg-[#141414] hover:bg-[#1a1a1a] border border-[#242424]"
                  >
                    {it.icon ? <span className="mr-2 opacity-70">{it.icon}</span> : null}
                    <span>{it.type}</span>
                  </button>
                ))}
              </div>
            </div>
          ))}
      </div>
    </div>
  );
}
