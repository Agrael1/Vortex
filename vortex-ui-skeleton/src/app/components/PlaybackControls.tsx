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
          className={`
            flex items-center justify-center w-8 h-8 rounded-md transition-colors
            ${disablePlay ? 'bg-green-600/20 text-green-400 cursor-not-allowed' : 'bg-green-600 hover:bg-green-700 text-white'}
          `}
          title="Play"
        >
          <svg width="12" height="12" viewBox="0 0 24 24" fill="currentColor">
            <path d="M8 5v14l11-7z" />
          </svg>
        </button>

        <button
          onClick={handleStop}
          disabled={disableStop}
          className={`
            flex items-center justify-center w-8 h-8 rounded-md transition-colors
            ${disableStop ? 'bg-red-600/20 text-red-400 cursor-not-allowed' : 'bg-red-600 hover:bg-red-700 text-white'}
          `}
          title="Stop"
        >
          <svg width="12" height="12" viewBox="0 0 24 24" fill="currentColor">
            <rect x="6" y="6" width="12" height="12" />
          </svg>
        </button>
      </div>

      {/* Status indicator */}
      <div
        className="flex items-center gap-2 px-3 py-1 rounded-md bg-black/20 border border-gray-600"
        title={transport.reason ?? undefined}
      >
        <div className="w-2 h-2 rounded-full" style={{ backgroundColor: getStatusColor() }} />
        <span className="text-sm text-gray-300">{getStatusText()}</span>
      </div>

      {/* FPS Counter */}
      <div className="flex items-center gap-2 px-3 py-1 rounded-md bg-black/20 border border-gray-600">
        <span className="text-sm text-gray-400">FPS:</span>
        <span
          className={`text-sm font-mono ${transport.fps < 55 ? 'text-red-400' : transport.fps < 58 ? 'text-yellow-400' : 'text-green-400'}`}
        >
          {transport.fps}
        </span>
      </div>

      {/* Dropped frames */}
      <div
        className="flex items-center gap-3 px-3 py-1 rounded-md bg-black/20 border border-gray-600"
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
