import { useEffect, useState } from 'react';
import { Vortex } from '@/bridge/vortex';

type Props = { selectedPtr: number | null };

export function InspectorPanel({ selectedPtr }: Props) {
  const [text, setText] = useState('Nothing selected');

  useEffect(() => {
    let cancelled = false;

    (async () => {
      if (!selectedPtr) { setText('Nothing selected'); return; }
      try {
        const raw = await Vortex.getNodeProperties(selectedPtr);
        const pretty = typeof raw === 'string'
          ? raw
          : JSON.stringify(raw, null, 2);
        if (!cancelled) setText(pretty);
      } catch (e: any) {
        if (!cancelled) setText(`Failed to load props: ${e?.message ?? e}`);
      }
    })();

    return () => { cancelled = true; };
  }, [selectedPtr]);

  return (
    <pre style={{
      margin: 0, padding: 8, height: '100%', overflow: 'auto',
      fontSize: 12, color: '#dcdcdc',
    }}>
      {text}
    </pre>
  );
}
