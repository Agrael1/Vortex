import { useCallback } from 'react';
import { GraphPanel } from '@/app/panels/GraphPanel';
import { NodeLibraryPanel } from '@/app/panels/NodeLibraryPanel';
import { InspectorPanel } from '@/app/panels/InspectorPanel';
import { engine } from '@/app/services/ipc/cefBridge';
import { useGraphCommands } from '@state/hooks/useGraphCommands';

const ensurePtr = (value: number | null, context: string): number => {
  if (typeof value !== 'number' || !Number.isFinite(value)) {
    throw new Error(`[${context}] Node pointer is not available`);
  }
  return value;
};

export function AppShell() {
  const { createNode, connectNodes, updateNodeProps } = useGraphCommands();

  const quickTestPattern = useCallback(async () => {
    try {
      console.log('[Quick Test] Starting Image→Window test pattern...');

      console.log('[Quick Test] Creating ImageInput node...');
      const imagePtr = await createNode('ImageInput', { x: 200, y: 200 });
      console.log('[Quick Test] ImageInput created with ptr:', imagePtr);

      console.log('[Quick Test] Creating WindowOutput node...');
      const windowPtr = await createNode('WindowOutput', { x: 520, y: 220 });
      console.log('[Quick Test] WindowOutput created with ptr:', windowPtr);

      console.log('[Quick Test] Connecting nodes...', imagePtr, '->', windowPtr);
      const imageHandle = ensurePtr(imagePtr, 'Quick Test image');
      const windowHandle = ensurePtr(windowPtr, 'Quick Test window');
      await connectNodes({ source: imageHandle, target: windowHandle });
      console.log('[Quick Test] Image→Window pipeline created successfully!');
      console.log('[Quick Test] Now click Play to see test pattern');
    } catch (error) {
      console.error('[Quick Test] Error creating Image→Window:', error);
    }
  }, [connectNodes, createNode]);
  const quickStreamToWindow = useCallback(async () => {
    try {
      console.log('[Quick] Starting Stream→Window creation...');

      console.log('[Quick] Creating StreamInput node...');
      const streamPtr = await createNode('StreamInput', { x: 200, y: 200 });
      console.log('[Quick] StreamInput created with ptr:', streamPtr);

      console.log('[Quick] Creating WindowOutput node...');
      const windowPtr = await createNode('WindowOutput', { x: 520, y: 220 });
      console.log('[Quick] WindowOutput created with ptr:', windowPtr);

      console.log('[Quick] Connecting nodes...', streamPtr, '->', windowPtr);
      const streamHandle = ensurePtr(streamPtr, 'Quick Stream source');
      const windowHandle = ensurePtr(windowPtr, 'Quick Stream target');
      await connectNodes({ source: streamHandle, target: windowHandle });
      console.log('[Quick] Stream→Window pipeline created successfully!');
    } catch (error) {
      console.error('[Quick] Error creating Stream→Window:', error);
    }
  }, [connectNodes, createNode]);

  const quickStreamToWindowWithVideo = useCallback(async () => {
    try {
      console.log('[Quick Video] Starting Stream→Window with video...');

      // Ask user for video file path
      const videoPath = prompt('Enter path to video file or stream URL:', 'C:\\path\\to\\video.mp4');
      if (!videoPath) {
        console.log('[Quick Video] User cancelled video selection');
        return;
      }

      console.log('[Quick Video] Creating StreamInput node...');
      const streamPtr = await createNode('StreamInput', { x: 200, y: 200 });
      console.log('[Quick Video] StreamInput created with ptr:', streamPtr);

      console.log('[Quick Video] Setting stream_url to:', videoPath);
      const streamHandle = ensurePtr(streamPtr, 'Quick Video stream');
      await updateNodeProps(streamHandle, { stream_url: videoPath });

      console.log('[Quick Video] Creating WindowOutput node...');
      const windowPtr = await createNode('WindowOutput', { x: 520, y: 220 });
      console.log('[Quick Video] WindowOutput created with ptr:', windowPtr);

      console.log('[Quick Video] Connecting nodes...', streamPtr, '->', windowPtr);
      const windowHandle = ensurePtr(windowPtr, 'Quick Video window');
      await connectNodes({ source: streamHandle, target: windowHandle });
      console.log('[Quick Video] Stream→Window pipeline created successfully!');
      console.log('[Quick Video] Now click Play button to start video playback');
    } catch (error) {
      console.error('[Quick Video] Error creating Stream→Window with video:', error);
    }
  }, [connectNodes, createNode, updateNodeProps]);

  const quickSampleVideo = useCallback(async () => {
    try {
      console.log('[Quick Sample] Starting sample video test...');

      // Use the sample video from public folder - corrected path
      const videoPath = './ui/ui/sample.mp4';

      console.log('[Quick Sample] Creating StreamInput node...');
      const streamPtr = await createNode('StreamInput', { x: 200, y: 200 });
      console.log('[Quick Sample] StreamInput created with ptr:', streamPtr);

      console.log('[Quick Sample] Setting stream_url to sample video:', videoPath);
      const streamHandle = ensurePtr(streamPtr, 'Quick Sample stream');
      await updateNodeProps(streamHandle, { stream_url: videoPath });

      console.log('[Quick Sample] Creating WindowOutput node...');
      const windowPtr = await createNode('WindowOutput', { x: 520, y: 220 });
      console.log('[Quick Sample] WindowOutput created with ptr:', windowPtr);

      console.log('[Quick Sample] Connecting nodes...', streamPtr, '->', windowPtr);
      const windowHandle = ensurePtr(windowPtr, 'Quick Sample window');
      await connectNodes({ source: streamHandle, target: windowHandle });
      console.log('[Quick Sample] Sample video pipeline created successfully!');
      console.log('[Quick Sample] Now click Play button to start sample video playback');
    } catch (error) {
      console.error('[Quick Sample] Error creating sample video pipeline:', error);
    }
  }, [connectNodes, createNode, updateNodeProps]);

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
      <div
        className="col-span-3"
        style={{
          display: 'flex',
          alignItems: 'center',
          gap: 8,
          padding: '0 12px',
          borderBottom: '1px solid #2a2a2a',
          background: '#101010',
        }}
      >
        <button
          onClick={() => {
            engine.play().catch((error) => console.error('[AppShell] Play failed', error));
          }}
          style={{ padding: '6px 12px', borderRadius: 6, background: '#116149', color: '#fff' }}
        >
          Play
        </button>
        <button
          onClick={() => {
            engine.stop().catch((error) => console.error('[AppShell] Stop failed', error));
          }}
          style={{ padding: '6px 12px', borderRadius: 6, background: '#7a1f2c', color: '#fff' }}
        >
          Stop
        </button>
        <button
          onClick={quickStreamToWindow}
          style={{ marginLeft: 12, padding: '6px 12px', borderRadius: 6, background: '#222', color: '#ddd', border: '1px solid #2a2a2a' }}
        >
          Quick: Stream→Window
        </button>
        <button
          onClick={quickStreamToWindowWithVideo}
          style={{ padding: '6px 12px', borderRadius: 6, background: '#2a4a2a', color: '#ddd', border: '1px solid #2a2a2a' }}
        >
          Video: Stream→Window
        </button>
        <button
          onClick={quickSampleVideo}
          style={{ padding: '6px 12px', borderRadius: 6, background: '#4a2a2a', color: '#ddd', border: '1px solid #2a2a2a' }}
        >
          Sample: Test Video
        </button>
        <button
          onClick={quickTestPattern}
          style={{ padding: '6px 12px', borderRadius: 6, background: '#1a3a3a', color: '#ddd', border: '1px solid #2a2a2a' }}
        >
          Test: Image→Window
        </button>
      </div>

      <div style={{ borderRight: '1px solid #2a2a2a', background: '#0c0c0c' }}>
        <NodeLibraryPanel />
      </div>

      <div style={{ minHeight: 0, background: '#111' }}>
        <GraphPanel />
      </div>

      <div style={{ borderLeft: '1px solid #2a2a2a', background: '#0c0c0c' }}>
        <div className="p-2 font-semibold" style={{ padding: 8 }}>
          Inspector
        </div>
        <InspectorPanel />
      </div>
    </div>
  );
}
