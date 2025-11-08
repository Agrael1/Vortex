// src/app/AppShell.tsx
import { useCallback, useState } from 'react';
import { GraphPanel } from '@/app/panels/GraphPanel';
import { NodeLibraryPanel } from '@/app/panels/NodeLibraryPanel';
import { InspectorPanel } from '@/app/panels/InspectorPanel';

export function AppShell() {
  const [selectedPtr, setSelectedPtr] = useState<number | null>(null);

  const onCreateNode = useCallback((typeName: string, ptr: number) => {
    (window as any).__GraphPanelAddNode?.(typeName, ptr);
  }, []);

  return (
    <div
      className="grid"
      style={{
        gridTemplateColumns: '260px 1fr 320px',
        gridTemplateRows: '1fr',
        width: '100vw',
        height: '100vh',
        minHeight: 0,
      }}
    >
      <div className="border-r bg-background">
        <NodeLibraryPanel onCreate={onCreateNode} />
      </div>

      <div className="bg-muted/30" style={{ minHeight: 0 }}>
        <GraphPanel onSelectPtr={setSelectedPtr} />
      </div>

      <div className="border-l bg-background">
        <div className="p-2 font-semibold">Inspector</div>
        <InspectorPanel selectedPtr={selectedPtr} />
      </div>
    </div>
  );
}
