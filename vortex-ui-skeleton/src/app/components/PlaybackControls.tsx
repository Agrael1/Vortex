import { useCallback, useEffect, useState } from 'react';
import { Vortex } from '@/bridge/vortex';

interface PlaybackControlsProps {
  className?: string;
}

export function PlaybackControls({ className = '' }: PlaybackControlsProps) {
  const [isPlaying, setIsPlaying] = useState(false);
  const [fps, setFps] = useState(60);
  const [status, setStatus] = useState<'ready' | 'playing' | 'stopped' | 'error'>('ready');

  const handlePlay = useCallback(async () => {
    try {
      setStatus('playing');
      await Vortex.play();
      setIsPlaying(true);
    } catch (error) {
      console.error('Failed to start playback:', error);
      setStatus('error');
      setIsPlaying(false);
    }
  }, []);

  const handleStop = useCallback(async () => {
    try {
      setStatus('stopped');
      await Vortex.stop();
      setIsPlaying(false);
    } catch (error) {
      console.error('Failed to stop playback:', error);
      setStatus('error');
    }
  }, []);

  // FPS monitoring (mock for now, can be enhanced with real data)
  useEffect(() => {
    const interval = setInterval(() => {
      // In a real implementation, this would come from the engine
      setFps(Math.round(58 + Math.random() * 4)); // 58-62 fps simulation
    }, 1000);

    return () => clearInterval(interval);
  }, []);

  const getStatusColor = () => {
    switch (status) {
      case 'playing':
        return '#22c55e'; // green
      case 'stopped':
        return '#ef4444'; // red
      case 'error':
        return '#f59e0b'; // amber
      default:
        return '#6b7280'; // gray
    }
  };

  const getStatusText = () => {
    switch (status) {
      case 'playing':
        return 'Playing';
      case 'stopped':
        return 'Stopped';
      case 'error':
        return 'Error';
      default:
        return 'Ready';
    }
  };

  return (
    <div className={`flex items-center gap-3 ${className}`}>
      {/* Play/Stop buttons */}
      <div className="flex items-center gap-2">
        <button
          onClick={handlePlay}
          disabled={isPlaying}
          className={`
            flex items-center justify-center w-8 h-8 rounded-md transition-colors
            ${isPlaying ? 'bg-green-600/20 text-green-400 cursor-not-allowed' : 'bg-green-600 hover:bg-green-700 text-white'}
          `}
          title="Play"
        >
          <svg width="12" height="12" viewBox="0 0 24 24" fill="currentColor">
            <path d="M8 5v14l11-7z" />
          </svg>
        </button>

        <button
          onClick={handleStop}
          disabled={!isPlaying}
          className={`
            flex items-center justify-center w-8 h-8 rounded-md transition-colors
            ${!isPlaying ? 'bg-red-600/20 text-red-400 cursor-not-allowed' : 'bg-red-600 hover:bg-red-700 text-white'}
          `}
          title="Stop"
        >
          <svg width="12" height="12" viewBox="0 0 24 24" fill="currentColor">
            <rect x="6" y="6" width="12" height="12" />
          </svg>
        </button>
      </div>

      {/* Status indicator */}
      <div className="flex items-center gap-2 px-3 py-1 rounded-md bg-black/20 border border-gray-600">
        <div className="w-2 h-2 rounded-full" style={{ backgroundColor: getStatusColor() }} />
        <span className="text-sm text-gray-300">{getStatusText()}</span>
      </div>

      {/* FPS Counter */}
      <div className="flex items-center gap-2 px-3 py-1 rounded-md bg-black/20 border border-gray-600">
        <span className="text-sm text-gray-400">FPS:</span>
        <span className={`text-sm font-mono ${fps < 55 ? 'text-red-400' : fps < 58 ? 'text-yellow-400' : 'text-green-400'}`}>{fps}</span>
      </div>

      {/* GPU Info */}
      <div className="text-xs text-gray-500">GPU: DX12</div>
    </div>
  );
}
