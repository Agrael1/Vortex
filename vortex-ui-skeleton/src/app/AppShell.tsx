import { useCallback, useState } from 'react';
import { GraphPanel } from '@/app/panels/GraphPanel';
import { NodeLibraryPanel } from '@/app/panels/NodeLibraryPanel';
import { InspectorPanel } from '@/app/panels/InspectorPanel';
import { Vortex } from '../bridge/vortex';;

export function AppShell() {
  const [selectedPtr, setSelectedPtr] = useState<number | null>(null);

  // клик в палитре
  const onCreateNode = useCallback((typeName: string) => {
    (window as any).__GraphPanelAddNode?.(typeName);
  }, []);

  // быстрый пресет StreamInput -> WindowOutput
  const quickStreamToWindow = useCallback(async () => {
    const add = (window as any).__GraphPanelAddNode as (t: string, x?: number, y?: number) => Promise<void> | void;
    if (!add) return;

    // создаём две ноды рядом
    // ptr вернётся внутри add, но для связи нам нужны ptr — получим через бэкенд свойство выбранной ноды?
    // Проще — после создания мы запросим последние props через инспекторные вызовы.
    // Для детерминированности — будем вызывать connect из бэкенда по id, которые мы уже знаем, если у тебя есть метод.
    // В текущем MVP — просто создадим, а соединить можно вручную (или заработает ConnectNodesAsync ниже).
    (window as any).__GraphPanelAddNode?.('StreamInput', 200, 200);
    (window as any).__GraphPanelAddNode?.('WindowOutput', 520, 220);

    // Если на бэке есть ConnectNodesAsync, попробуем автоконнектить.
    // Мы не знаем ptr вновь созданных нод здесь напрямую — обычно их лучше хранить в состоянии GraphPanel.
    // Для MVP оставим соединение вручную; при необходимости позже добавим обмен событиями.
  }, []);

  return (
    <div
      className="grid"
      style={{
        gridTemplateColumns: '260px 1fr 340px',
        gridTemplateRows: '40px 1fr',
        width: '100vw',
        height: '100vh',
        minHeight: 0,
      }}
    >
      {/* top bar */}
      <div className="col-span-3" style={{ display: 'flex', alignItems: 'center', gap: 8, padding: '0 12px', borderBottom: '1px solid #2a2a2a', background: '#101010' }}>
        <button onClick={() => Vortex.play()}  style={{ padding: '6px 12px', borderRadius: 6, background: '#116149', color: '#fff' }}>Play</button>
        <button onClick={() => Vortex.stop()}  style={{ padding: '6px 12px', borderRadius: 6, background: '#7a1f2c', color: '#fff' }}>Stop</button>
        <button onClick={quickStreamToWindow} style={{ marginLeft: 12, padding: '6px 12px', borderRadius: 6, background: '#222', color: '#ddd', border: '1px solid #2a2a2a' }}>
          Quick: Stream→Window
        </button>
      </div>

      <div style={{ borderRight: '1px solid #2a2a2a', background: '#0c0c0c' }}>
        <NodeLibraryPanel onCreate={onCreateNode} />
      </div>

      <div style={{ minHeight: 0, background: '#111' }}>
        <GraphPanel onSelectPtr={setSelectedPtr} />
      </div>

      <div style={{ borderLeft: '1px solid #2a2a2a', background: '#0c0c0c' }}>
        <InspectorPanel selectedPtr={selectedPtr} />
      </div>
    </div>
  );
}
