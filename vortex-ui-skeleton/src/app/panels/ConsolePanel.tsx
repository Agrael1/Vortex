import React, { useEffect } from 'react';
import { useRecoilValue } from 'recoil';
import { consoleEntriesAtom } from '@state/atoms/editor';
import { useConsoleFeed } from '@state/hooks/useConsoleFeed';
import { engine } from '@/app/services/ipc/cefBridge';

const levelStyles: Record<string, string> = {
  info: 'text-slate-200',
  warn: 'text-yellow-300',
  error: 'text-red-400',
};

export function ConsolePanel() {
  const entries = useRecoilValue(consoleEntriesAtom);
  const { clearLogs } = useConsoleFeed();

  useEffect(() => {
    if (entries.length > 0) return;
    const timers = [
      setTimeout(() => engine.emitLog({ level: 'info', message: 'Engine ready', time: Date.now(), scope: 'runtime' }), 300),
      setTimeout(() => engine.emitLog({ level: 'info', message: 'Graph compiled', time: Date.now(), scope: 'graph' }), 800),
    ];
    return () => {
      timers.forEach(clearTimeout);
    };
  }, [entries.length]);

  return (
    <div className="w-full h-full flex flex-col text-xs font-mono bg-black/70">
      <div className="flex items-center justify-between px-3 py-2 border-b border-ui-border text-gray-300">
        <span>Console · {entries.length}</span>
        <button type="button" onClick={clearLogs} className="text-gray-400 hover:text-white transition-colors text-[11px]">
          Clear
        </button>
      </div>
      <div className="flex-1 overflow-auto px-3 py-2 space-y-1">
        {entries.map((entry) => (
          <div key={entry.id} className={`flex gap-2 ${levelStyles[entry.level] ?? levelStyles.info}`}>
            <span className="text-gray-500 w-16 shrink-0">{new Date(entry.time).toLocaleTimeString()}</span>
            {entry.scope ? <span className="text-gray-500">[{entry.scope}]</span> : null}
            <span className="flex-1 whitespace-pre-wrap break-all">{entry.message}</span>
          </div>
        ))}
        {entries.length === 0 ? <div className="text-gray-500">No messages yet…</div> : null}
      </div>
    </div>
  );
}
