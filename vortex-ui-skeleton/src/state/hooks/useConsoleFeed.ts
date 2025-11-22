import { useEffect } from 'react';
import { useRecoilCallback, useSetRecoilState } from 'recoil';
import { consoleEntriesAtom } from '@state/atoms/editor';
import type { ConsoleEntry } from '@state/types';
import { engine, type EngineLogEntry } from '@/app/services/ipc/cefBridge';

const DEFAULT_LIMIT = 500;

export function useConsoleFeed(limit: number = DEFAULT_LIMIT) {
  const setEntries = useSetRecoilState(consoleEntriesAtom);

  useEffect(() => {
    let counter = 0;

    const handler = (entry: EngineLogEntry | ConsoleEntry | unknown) => {
      const normalized: ConsoleEntry = {
        id: (entry as ConsoleEntry).id ?? `${Date.now()}-${counter++}`,
        level: (entry as EngineLogEntry).level ?? 'info',
        message: (entry as EngineLogEntry).message ?? '',
        time: (entry as EngineLogEntry).time ?? Date.now(),
        scope: (entry as ConsoleEntry).scope ?? (entry as EngineLogEntry).scope ?? null,
      };

      setEntries((prev) => {
        const next = [...prev, normalized];
        if (next.length > limit) {
          return next.slice(next.length - limit);
        }
        return next;
      });
    };

    const off = engine.on<EngineLogEntry>('log', handler);
    return () => {
      off?.();
    };
  }, [limit, setEntries]);

  const clearLogs = useRecoilCallback(
    ({ reset }) =>
      () => {
        reset(consoleEntriesAtom);
      },
    [],
  );

  return { clearLogs };
}
