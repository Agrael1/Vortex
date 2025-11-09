// src/app/panels/InspectorPanel.tsx
import { useEffect, useMemo, useState } from 'react';
import { Vortex } from '@/bridge/vortex';

export function InspectorPanel({ selectedPtr }: { selectedPtr: number | null }) {
  const [propsObj, setPropsObj] = useState<any>(null);

  useEffect(() => {
    if (selectedPtr == null) { setPropsObj(null); return; }
    Vortex.getNodeProps(selectedPtr).then(setPropsObj).catch(console.warn);
  }, [selectedPtr]);

  const keys = useMemo(() => (propsObj ? Object.keys(propsObj) : []), [propsObj]);

  if (selectedPtr == null) {
    return <div style={{ padding: 8, opacity: 0.7 }}>Nothing selected</div>;
  }

  const onChange = (k: string, v: string) => {
    let parsed: any = v;
    if (v === 'true' || v === 'false') parsed = v === 'true';
    else if (!Number.isNaN(Number(v)) && v.trim() !== '') parsed = Number(v);

    setPropsObj((prev: any) => ({ ...prev, [k]: parsed }));
    Vortex.setNodeProp(selectedPtr, k, parsed);
  };

  return (
    <div style={{ padding: 8, fontSize: 12 }}>
      <div style={{ fontWeight: 600, marginBottom: 6 }}>Inspector</div>
      <div style={{ marginBottom: 8, opacity: 0.8 }}>ptr: {selectedPtr}</div>

      {keys.length === 0 && <div style={{ opacity: 0.7 }}>No properties</div>}

      {keys.map((k) => (
        <div key={k} style={{ marginBottom: 8 }}>
          <div style={{ marginBottom: 4, opacity: 0.8 }}>{k}</div>
          <input
            value={String(propsObj[k] ?? '')}
            onChange={(e) => onChange(k, e.target.value)}
            style={{ width: '100%' }}
          />
        </div>
      ))}
    </div>
  );
}
