// src/app/panels/InspectorPanel.tsx
import { useEffect, useState } from 'react';
import { engine, type PropSchema, type PropSpec } from '@/bridge/engine';

export function InspectorPanel({ selectedPtr }: { selectedPtr: number | null }) {
  const [schema, setSchema] = useState<PropSchema>({ properties: [] });

  useEffect(() => {
    let alive = true;
    (async () => {
      if (!selectedPtr) {
        if (alive) setSchema({ properties: [] });
        return;
      }
      try {
        const s = await engine.getNodeProperties(selectedPtr);
        if (alive) setSchema(s);
      } catch (e) {
        console.error('getNodeProperties failed', e);
        if (alive) setSchema({ properties: [] });
      }
    })();
    return () => {
      alive = false;
    };
  }, [selectedPtr]);

  const setValue = (p: PropSpec, v: any) => {
    if (!selectedPtr) return;
    // по имени надёжнее (индексы тоже есть)
    engine.setNodePropertyByName(selectedPtr, p.name, v);
    // локально сразу обновим UI
    setSchema((old) => ({
      properties: old.properties.map((x) => (x.name === p.name ? { ...x, value: v } : x)),
    }));
  };

  if (!selectedPtr) {
    return <div className="p-2 text-xs text-gray-400">Nothing selected</div>;
  }

  return (
    <div className="p-2 space-y-2 text-xs">
      {schema.properties.length === 0 && (
        <div className="text-gray-400">No properties</div>
      )}
      {schema.properties.map((p) => {
        const label = p.label ?? p.name;
        switch (p.type) {
          case 'bool':
            return (
              <label key={p.name} className="flex items-center gap-2">
                <input
                  type="checkbox"
                  checked={Boolean(p.value)}
                  onChange={(e) => setValue(p, e.target.checked)}
                />
                {label}
              </label>
            );
          case 'int':
          case 'float':
            return (
              <div key={p.name} className="space-y-1">
                <div className="text-gray-400">{label}</div>
                <input
                  type="number"
                  className="w-full bg-[#151515] border border-[#2a2a2a] rounded px-2 py-1 outline-none text-gray-200"
                  value={p.value ?? p.default ?? 0}
                  step={p.step ?? (p.type === 'int' ? 1 : 0.01)}
                  onChange={(e) => setValue(p, Number(e.target.value))}
                />
              </div>
            );
          case 'enum':
            return (
              <div key={p.name} className="space-y-1">
                <div className="text-gray-400">{label}</div>
                <select
                  className="w-full bg-[#151515] border border-[#2a2a2a] rounded px-2 py-1 outline-none text-gray-200"
                  value={p.value ?? p.default ?? (p.enum?.[0]?.value ?? '')}
                  onChange={(e) => setValue(p, e.target.value)}
                >
                  {(p.enum ?? []).map((opt, i) => (
                    <option key={i} value={opt.value}>
                      {opt.label}
                    </option>
                  ))}
                </select>
              </div>
            );
          default:
            return (
              <div key={p.name} className="space-y-1">
                <div className="text-gray-400">{label}</div>
                <input
                  className="w-full bg-[#151515] border border-[#2a2a2a] rounded px-2 py-1 outline-none text-gray-200"
                  value={p.value ?? p.default ?? ''}
                  onChange={(e) => setValue(p, e.target.value)}
                />
              </div>
            );
        }
      })}
    </div>
  );
}
