// Универсальный бридж: сначала ищем window.Vortex.callAsync (встроенный прокси),
// если его нет — используем сырой cefQuery.
type Req = { name: string; payload?: any };

function nativeCall(name: string, payload?: any): Promise<string> {
  const proxied = (window as any).Vortex?.callAsync;
  if (proxied) return proxied(name, payload);

  const cefQuery = (window as any).cefQuery;
  if (!cefQuery) return Promise.reject(new Error('Vortex bridge not found'));

  const req: Req = { name, payload };
  return new Promise((resolve, reject) => {
    cefQuery({
      request: JSON.stringify(req),
      onSuccess: (r: string) => resolve(r),
      onFailure: (_code: number, msg: string) =>
        reject(new Error(msg || 'cefQuery failed')),
    });
  });
}

// Безопасный вызов: если метода нет на бэке, просто логируем и не роняем UI.
async function tryCall(name: string, payload?: any) {
  try {
    return await nativeCall(name, payload);
  } catch (e) {
    console.warn(`[Vortex] call ${name} failed:`, e);
    return undefined;
  }
}

export const Vortex = {
  async getNodeTypes(): Promise<string[]> {
    const r = await nativeCall('GetNodeTypesAsync');
    return JSON.parse(r);
  },
  async createNode(type: string): Promise<number> {
    const r = await nativeCall('CreateNodeAsync', { type });
    return Number(r);
  },
  async connect(srcPtr: number, outIdx: number, dstPtr: number, inIdx: number): Promise<void> {
    await tryCall('ConnectNodesAsync', { srcPtr, outIdx, dstPtr, inIdx });
  },
  async getNodeProps(ptr: number): Promise<any> {
    const r = await nativeCall('GetNodePropertiesAsync', { ptr });
    return JSON.parse(r);
  },
  async setNodeProp(ptr: number, key: string, value: any): Promise<void> {
    await tryCall('SetNodePropertyAsync', { ptr, key, value });
  },
  play() { return tryCall('Play'); },
  stop() { return tryCall('Stop'); },
};
