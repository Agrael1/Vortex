type BridgeFn = (name: string, ...args: any[]) => Promise<any> | any;

let _bridge: BridgeFn | null = null;

function tryResolveBridge(): BridgeFn {
  const w = window as any;

  console.log('[Bridge] Checking available functions...');
  console.log('[Bridge] vortexCall:', typeof w.vortexCall);
  console.log('[Bridge] vortexCallAsync:', typeof w.vortexCallAsync);
  console.log('[Bridge] VortexBridge:', typeof w.VortexBridge);
  console.log('[Bridge] cefQuery:', typeof w.cefQuery);

  // 1) CEF V8 bridge - check availability of vortexCall and vortexCallAsync
  if (typeof w.vortexCall === 'function' && typeof w.vortexCallAsync === 'function') {
    console.log('[Bridge] CEF functions found: vortexCall, vortexCallAsync');
    return (name: string, ...args: any[]) => {
      try {
        // Use vortexCallAsync for promises
        if (args.length === 0) {
          return w.vortexCallAsync(name);
        }
        // Pass arguments separately
        return w.vortexCallAsync(name, ...args);
      } catch (error) {
        console.error('[Bridge] CEF call error:', error);
        return Promise.reject(error);
      }
    };
  }

  // 2) Fallback - legacy VortexBridge approach
  if (typeof w.VortexBridge === 'function') {
    console.log('[Bridge] Legacy VortexBridge found');
    return (name: string, ...args: any[]) => {
      try {
        const result = w.VortexBridge(name, ...args);
        if (result && typeof result.then === 'function') {
          return result;
        }
        return Promise.resolve(result);
      } catch (error) {
        return Promise.reject(error);
      }
    };
  }

  // 3) Proxy approach through CEF message system  
  if (typeof w.VortexCall === 'undefined') {
    w.VortexCall = (method: string, ...args: any[]) => {
      if (typeof w.cefQuery === 'function') {
        return new Promise((resolve, reject) => {
          w.cefQuery({
            request: JSON.stringify({ method, args }),
            onSuccess: (resp: string) => {
              try { resolve(JSON.parse(resp)); } catch { resolve(resp); }
            },
            onFailure: (_: number, msg: string) => reject(new Error(msg)),
          });
        });
      }
      console.error('[Bridge] No CEF functions available');
      throw new Error('CEF bridge not available');
    };
  }

  return (name: string, ...args: any[]) => {
    return w.VortexCall(name, ...args);
  };
}

async function ensureBridge(timeoutMs = 4000): Promise<BridgeFn> {
  if (_bridge) return _bridge;
  const t0 = Date.now();
  
  console.log('[Bridge] Searching for bridge functions...');
  console.log('[Bridge] Available window functions:', Object.getOwnPropertyNames(window).filter(name => typeof (window as any)[name] === 'function'));
  
  while (Date.now() - t0 < timeoutMs) {
    try { 
      _bridge = tryResolveBridge(); 
      console.log('[Bridge] Bridge found successfully!');
      return _bridge; 
    }
    catch (error) { 
      console.log('[Bridge] Bridge not found, retrying...', error);
      await new Promise(r => setTimeout(r, 50)); 
    }
  }
  throw new Error('Vortex bridge not found');
}

async function call(name: string, ...args: any[]) {
  const br = await ensureBridge();
  const res = await br(name, ...args);
  if (typeof res === 'string') { try { return JSON.parse(res); } catch {} }
  return res;
}

export const Vortex = {
  play:  () => call('Play'),
  stop:  () => call('Stop'),

  getNodeTypes:    () => call('GetNodeTypesAsync'),
  createNode:      (type: string) => call('CreateNodeAsync', type),
  connect:         (src: number, srcIdx: number, dst: number, dstIdx: number) =>
                    call('ConnectNodesAsync', src, srcIdx, dst, dstIdx),
  getNodeProperties: (ptr: number) => call('GetNodePropertiesAsync', ptr),
  setNodeProperty:   (ptr: number, keyOrIndex: string | number, value: any) => {
    if (typeof keyOrIndex === 'string') {
      return call('SetNodePropertyByName', ptr, keyOrIndex, value);
    } else {
      return call('SetNodeProperty', ptr, keyOrIndex, value);
    }
  },
  removeNode:       (ptr: number) => call('RemoveNode', ptr),
  disconnect:       (src: number, srcIdx: number, dst: number, dstIdx: number) =>
                     call('DisconnectNodes', src, srcIdx, dst, dstIdx),
};
