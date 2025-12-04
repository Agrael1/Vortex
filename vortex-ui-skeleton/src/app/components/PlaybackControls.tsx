import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { useRecoilValue, useSetRecoilState } from 'recoil';
import { engine } from '@/app/services/ipc/cefBridge';
import { useTransportState } from '@/state/hooks/useTransportState';
import { selectedNodePtrAtom } from '@/state/atoms/editor';

interface PlaybackControlsProps {
  className?: string;
}

export function PlaybackControls({ className = '' }: PlaybackControlsProps) {
  const transport = useTransportState();
  const [pendingAction, setPendingAction] = useState<'play' | 'stop' | null>(null);
  const selectedPtr = useRecoilValue(selectedNodePtrAtom);
  const setSelectedPtr = useSetRecoilState(selectedNodePtrAtom);
  const dropValue = typeof transport.droppedFrames === 'number' ? transport.droppedFrames : null;
  const dropTarget = transport.dropTarget && transport.dropTarget.trim().length ? transport.dropTarget : null;
  const dropTargetIdHex = typeof transport.dropTargetId === 'number' && Number.isFinite(transport.dropTargetId)
    ? `0x${Math.round(transport.dropTargetId).toString(16)}`
    : null;
  const dropTargetId = useMemo(() => {
    if (typeof transport.dropTargetId === 'number' && Number.isFinite(transport.dropTargetId)) {
      return Math.round(transport.dropTargetId);
    }
    return null;
  }, [transport.dropTargetId]);
  const lastAutoFocusedId = useRef<number | null>(null);

  const handlePlay = useCallback(async () => {
    try {
      setPendingAction('play');
      await engine.play();
    } catch (error) {
      console.error('Failed to start playback:', error);
    } finally {
      setPendingAction(null);
    }
  }, []);

  const handleStop = useCallback(async () => {
    try {
      setPendingAction('stop');
      await engine.stop();
    } catch (error) {
      console.error('Failed to stop playback:', error);
    } finally {
      setPendingAction(null);
    }
  }, []);

  const getStatusColor = () => {
    switch (transport.status) {
      case 'playing':
        return '#22c55e'; // green
      case 'paused':
        return '#f97316'; // orange
      case 'error':
        return '#f59e0b'; // amber
      default:
        return '#6b7280'; // gray
    }
  };

  const getStatusText = () => {
    if (transport.status === 'playing') return 'Playing';
    if (transport.status === 'paused') return 'Paused';
    if (transport.status === 'error') return 'Error';
    return 'Ready';
  };

  const getDropColorClass = () => {
    if (dropValue == null) return 'text-gray-400';
    if (dropValue === 0) return 'text-green-400';
    if (dropValue <= 2) return 'text-yellow-400';
    return 'text-red-400';
  };

  const disablePlay = transport.isPlaying || pendingAction === 'play';
  const disableStop = !transport.isPlaying || pendingAction === 'stop';
  const canFocusDropTarget = transport.hasRecentDrops && dropTargetId != null;
  const buttonBase =
    'relative flex items-center justify-center w-9 h-9 rounded-xl border transition-all duration-200 focus:outline-none focus-visible:ring-2 focus-visible:ring-[#8fd3ff]/70 focus-visible:ring-offset-2 focus-visible:ring-offset-[#030712] shadow-[0_12px_30px_rgba(3,7,18,0.55)]';
  const playButtonClass = disablePlay
    ? `${buttonBase} border-cyan-400/20 text-cyan-200/60 cursor-not-allowed`
    : `${buttonBase} border-transparent bg-gradient-to-br from-[#8FD3FF] via-[#7AA7FF] to-[#5B8CFF] text-[#04060c] hover:-translate-y-0.5 hover:shadow-[0_18px_40px_rgba(4,10,24,0.65)]`;
  const stopButtonClass = disableStop
    ? `${buttonBase} border-pink-400/20 text-pink-200/60 cursor-not-allowed`
    : `${buttonBase} border-transparent bg-gradient-to-br from-[#F472B6] via-[#FB7185] to-[#FD8BA3] text-[#280712] hover:-translate-y-0.5 hover:shadow-[0_18px_40px_rgba(12,2,12,0.55)]`;

  const handleFocusDropTarget = useCallback(() => {
    if (dropTargetId != null && dropTargetId !== selectedPtr) {
      setSelectedPtr(dropTargetId);
    }
  }, [dropTargetId, selectedPtr, setSelectedPtr]);

  useEffect(() => {
    if (!transport.hasRecentDrops || dropTargetId == null) {
      return;
    }

    if (lastAutoFocusedId.current === dropTargetId) {
      return;
    }

    if (dropTargetId === selectedPtr) {
      lastAutoFocusedId.current = dropTargetId;
      return;
    }

    lastAutoFocusedId.current = dropTargetId;
    setSelectedPtr(dropTargetId);
  }, [transport.hasRecentDrops, dropTargetId, selectedPtr, setSelectedPtr]);

  return (
    <div className={`flex items-center gap-3 ${className}`}>
      {/* Play/Stop buttons */}
      <div className="flex items-center gap-2">
        <button
          onClick={handlePlay}
          disabled={disablePlay}
            className={playButtonClass}
          title="Play"
        >
          <svg width="12" height="12" viewBox="0 0 24 24" fill="currentColor">
            <path d="M8 5v14l11-7z" />
          </svg>
        </button>

        <button
          onClick={handleStop}
          disabled={disableStop}
            className={stopButtonClass}
          title="Stop"
        >
          <svg width="12" height="12" viewBox="0 0 24 24" fill="currentColor">
            <rect x="6" y="6" width="12" height="12" />
          </svg>
        </button>
      </div>

      {/* Status indicator */}
      <div
        className="flex items-center gap-2 px-3 py-1.5 rounded-xl bg-white/5 border border-ui-border/60 backdrop-blur"
        title={transport.reason ?? undefined}
      >
        <div className="w-2 h-2 rounded-full" style={{ backgroundColor: getStatusColor() }} />
        <span className="text-sm text-gray-300">{getStatusText()}</span>
      </div>

      {/* FPS Counter */}
      <div className="flex items-center gap-2 px-3 py-1.5 rounded-xl bg-white/5 border border-ui-border/60 backdrop-blur">
        <span className="text-sm text-gray-400">FPS:</span>
        <span
          className={`text-sm font-mono ${transport.fps < 55 ? 'text-red-400' : transport.fps < 58 ? 'text-yellow-400' : 'text-green-400'}`}
        >
          {transport.fps}
        </span>
      </div>

      {/* Dropped frames */}
      <div
        className="flex items-center gap-3 px-3 py-1.5 rounded-xl bg-white/5 border border-ui-border/60 backdrop-blur"
        title={transport.hasRecentDrops ? transport.reason ?? dropTarget ?? undefined : 'Нет зафиксированных дропов'}
      >
        <div className="flex items-center gap-2">
          <span className="text-sm text-gray-400">Dropped:</span>
          <span className={`text-sm font-mono ${getDropColorClass()}`}>
            {dropValue == null ? '—' : dropValue}
          </span>
        </div>
        {transport.hasRecentDrops && (dropTarget || dropTargetIdHex) && (
          <span
            className="text-xs text-gray-400 truncate max-w-[180px]"
            title={[dropTarget, dropTargetIdHex].filter(Boolean).join(' ')}
          >
            {[dropTarget, dropTargetIdHex].filter(Boolean).join(' · ')}
          </span>
        )}
        {canFocusDropTarget && (
          <button
            type="button"
            onClick={handleFocusDropTarget}
            className="text-xs font-medium text-blue-300 hover:text-blue-200 underline-offset-2 hover:underline"
            title="Выделить проблемный выход"
          >
            Focus
          </button>
        )}
      </div>

      {/* GPU Info */}
      <div className="text-xs text-gray-500">GPU: DX12</div>
    </div>
  );
}
