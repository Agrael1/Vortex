import { useEffect, useRef } from 'react';
import { useRecoilValue } from 'recoil';
import { persistenceStatusAtom } from '@state/atoms/persistence';
import { engine, type EngineLogEntry } from '@/app/services/ipc/cefBridge';
import { useNotificationCenter } from '@state/hooks/useNotificationCenter';

const ENGINE_NOTIFICATION_DURATION = 8000;
const PERSISTENCE_NOTIFICATION_DURATION = 6000;

export function useEngineAlerts() {
  const status = useRecoilValue(persistenceStatusAtom);
  const { push } = useNotificationCenter();
  const lastPersistenceErrorRef = useRef<string | null>(null);
  const lastExternalChangeRef = useRef<string | null>(null);

  useEffect(() => {
    const off = engine.on<EngineLogEntry>('log', (entry) => {
      if (!entry || entry.level !== 'error') {
        return;
      }

      push({
        level: 'error',
        title: entry.scope ?? 'Engine',
        message: entry.message ?? 'Unknown engine error',
        scope: entry.scope ?? null,
        durationMs: ENGINE_NOTIFICATION_DURATION,
      });
    });

    return () => {
      off?.();
    };
  }, [push]);

  useEffect(() => {
    const currentError = status.lastError ?? null;
    if (currentError && currentError !== lastPersistenceErrorRef.current) {
      push({
        level: 'error',
        title: 'Persistence',
        message: currentError,
        durationMs: PERSISTENCE_NOTIFICATION_DURATION,
      });
    }
    lastPersistenceErrorRef.current = currentError;
  }, [push, status.lastError]);

  useEffect(() => {
    if (!status.hasExternalChange || !status.lastExternalChangeAt) {
      return;
    }

    if (status.lastExternalChangeAt === lastExternalChangeRef.current) {
      return;
    }

    lastExternalChangeRef.current = status.lastExternalChangeAt;
    push({
      level: 'warn',
      title: 'Project file changed on disk',
      message: 'Another process modified the current project. Review the changes before continuing.',
      durationMs: PERSISTENCE_NOTIFICATION_DURATION,
    });
  }, [push, status.hasExternalChange, status.lastExternalChangeAt]);
}
