import { useEffect, useMemo, useState } from 'react';
import { Vortex } from '../../bridge/vortex';
import { DND_TYPE } from './GraphPanel';

type Props = {
  onCreate?: (type: string) => void; // по клику
};

export function NodeLibraryPanel({ onCreate }: Props) {
  const [types, setTypes] = useState<string[]>([]);
  const [q, setQ] = useState('');

  useEffect(() => { Vortex.getNodeTypes().then(setTypes).catch(console.warn); }, []);
  const filtered = useMemo(
    () => types.filter(t => t.toLowerCase().includes(q.toLowerCase())),
    [types, q],
  );

  const onDragStart = (ev: React.DragEvent, type: string) => {
    ev.dataTransfer.setData(DND_TYPE, type);
    ev.dataTransfer.effectAllowed = 'move';
  };

  return (
    <div style={{ padding: 8 }}>
      <div style={{ fontWeight: 600, marginBottom: 6 }}>Nodes</div>
      <input
        placeholder="Search…"
        value={q}
        onChange={e => setQ(e.target.value)}
        style={{ width: '100%', marginBottom: 8 }}
      />
      <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
        {filtered.map(t => (
          <button
            key={t}
            onClick={() => onCreate?.(t)}
            draggable
            onDragStart={(e) => onDragStart(e, t)}
            title="Drag to canvas or click to add"
            style={{
              textAlign: 'left',
              padding: '6px 8px',
              borderRadius: 6,
              background: '#1a1a1a',
              color: '#ddd',
              border: '1px solid #2a2a2a',
            }}
          >
            {t}
          </button>
        ))}
      </div>
    </div>
  );
}
