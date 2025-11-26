import { useEffect, useRef, useState } from 'react';
import { engine, type TransportStatePayload, type TransportStatus } from '@/app/services/ipc/cefBridge';

const FALLBACK_FPS = 60;
const EVENT_STALE_THRESHOLD_MS = 4000;
const MOCK_FPS_MIN = 58;
const MOCK_FPS_MAX = 62;

type TransportSnapshot = {
  status: TransportStatus;
  fps: number;
  droppedFrames?: number | null;
  reason?: string | null;
  timestamp?: string | null;
  dropTarget?: string | null;
  dropTargetId?: number | null;
};

const clampFps = (value: unknown, previous: number): number => {
  if (typeof value === 'number' && Number.isFinite(value)) {
    return Math.max(0, Math.round(value));
  }

  return previous;
};

const sampleMockFps = () => {
  const span = MOCK_FPS_MAX - MOCK_FPS_MIN;
  return MOCK_FPS_MIN + Math.round(Math.random() * span);
};

export function useTransportState() {
  const [snapshot, setSnapshot] = useState<TransportSnapshot>({ status: 'ready', fps: FALLBACK_FPS });
  const lastEventRef = useRef<number>(0);
  const lastDropIdRef = useRef<string | null>(null);

  useEffect(() => {
    const unsubscribe = engine.on<TransportStatePayload>('transport:state', (payload) => {
      lastEventRef.current = Date.now();
      setSnapshot((prev) => ({
        status: payload?.state ?? prev.status,
        fps: clampFps(payload?.fps, prev.fps),
        droppedFrames: typeof payload?.droppedFrames === 'number' ? payload.droppedFrames : prev.droppedFrames ?? null,
        reason: payload?.reason ?? null,
        timestamp: payload?.timestamp ?? new Date().toISOString(),
        dropTarget: payload?.dropTarget ?? null,
        dropTargetId: typeof payload?.dropTargetId === 'number' && Number.isFinite(payload.dropTargetId)
          ? payload.dropTargetId
          : null,
      }));

      const hasDrop = typeof payload?.droppedFrames === 'number' && payload.droppedFrames > 0;
      if (hasDrop) {
        const dropIdentifier = `${payload?.dropTarget ?? ''}-${payload?.dropTargetId ?? ''}`;
        if (dropIdentifier && dropIdentifier !== lastDropIdRef.current) {
          lastDropIdRef.current = dropIdentifier;
          const message = payload?.dropTarget
            ? `Dropped frames detected on ${payload.dropTarget} (${payload.droppedFrames})`
            : `Dropped frames detected (${payload.droppedFrames})`;
          engine.emitLog?.({
            level: 'warn',
            message,
            time: Date.now(),
            scope: 'transport',
          });
        }
      }
    });

    if (typeof window === 'undefined') {
      return () => {
        unsubscribe?.();
      };
    }

    const interval = window.setInterval(() => {
      if (Date.now() - lastEventRef.current < EVENT_STALE_THRESHOLD_MS) {
        return;
      }

      setSnapshot((prev) => ({
        ...prev,
        fps: sampleMockFps(),
      }));
    }, 1000);

    return () => {
      unsubscribe?.();
      window.clearInterval(interval);
    };
  }, []);

  return {
    status: snapshot.status,
    fps: snapshot.fps,
    droppedFrames: snapshot.droppedFrames ?? null,
    reason: snapshot.reason ?? null,
    timestamp: snapshot.timestamp ?? null,
    dropTarget: snapshot.dropTarget ?? null,
    dropTargetId: snapshot.dropTargetId ?? null,
    isPlaying: snapshot.status === 'playing',
    isErrored: snapshot.status === 'error',
    hasRecentDrops: typeof snapshot.droppedFrames === 'number' && snapshot.droppedFrames > 0,
  };
}
