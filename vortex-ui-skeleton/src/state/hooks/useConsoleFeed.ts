import { useEffect } from 'react';
import { useRecoilCallback, useSetRecoilState } from 'recoil';
import { consoleEntriesAtom } from '@state/atoms/editor';
import type { ConsoleEntry } from '@state/types';
import { engine, type EngineLogEntry } from '@/app/services/ipc/cefBridge';

const DEFAULT_LIMIT = 500;

const formatConsoleValue = (value: unknown): string => {
  if (typeof value === 'string') return value;
  if (value instanceof Error) {
    return `${value.name}: ${value.message}${value.stack ? `\n${value.stack}` : ''}`;
  }
  try {
    return JSON.stringify(value);
  } catch {
    return String(value);
  }
};

const patchBrowserConsole = (() => {
  let patched = false;

  return () => {
    if (patched || typeof window === 'undefined') return;
    const native = {
      log: window.console.log.bind(window.console),
      warn: window.console.warn.bind(window.console),
      error: window.console.error.bind(window.console),
    };

    const forward = (level: EngineLogEntry['level'], args: unknown[]) => {
      try {
        (level === 'info' ? native.log : level === 'warn' ? native.warn : native.error)(...args);
      } catch {
        native.log(...args);
      }

      const message = args.map((arg) => formatConsoleValue(arg)).join(' ');
      engine.emitLog({ level, message, scope: 'console', time: Date.now() });
    };

    window.console.log = (...args: unknown[]) => forward('info', args);
    window.console.warn = (...args: unknown[]) => forward('warn', args);
    window.console.error = (...args: unknown[]) => forward('error', args);
    patched = true;
  };
})();

const useConsoleSubscription = (limit: number) => {
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
};

export function useConsoleBridge(limit: number = DEFAULT_LIMIT) {
  useConsoleSubscription(limit);

  useEffect(() => {
    patchBrowserConsole();
  }, []);
}

export function useConsoleActions() {
  const clearLogs = useRecoilCallback(
    ({ reset }) =>
      () => {
        reset(consoleEntriesAtom);
      },
    [],
  );

  return { clearLogs };
}
